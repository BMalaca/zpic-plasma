/*
 *  emf.c
 *  zpic
 *
 *  Created by Ricardo Fonseca on 10/8/10.
 *  Copyright 2010 Centro de Física dos Plasmas. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "emf.h"
#include "zdf.h"
#include "timer.h"

static double _emf_time = 0.0;

double emf_time( void )
{
	return _emf_time;
}

/*********************************************************************************************

 Constructor / Destructor

 *********************************************************************************************/

void emf_new( t_emf *emf, int nx, t_fld box, const float dt )
{

	// Number of guard cells for linear interpolation
	int gc[2] = {1,2};

	// Allocate global arrays
	size_t size = (gc[0] + nx + gc[1]) * sizeof( t_vfld ) ;

	emf->E_buf = malloc( size );
	emf->B_buf = malloc( size );

	assert( emf->E_buf && emf->B_buf );

	// zero fields
	memset( emf->E_buf, 0, size );
	memset( emf->B_buf, 0, size );

	// store nx and gc values
	emf->nx = nx;
	emf->gc[0] = gc[0];
	emf->gc[1] = gc[1];

    // store time step values
    emf -> dt = dt;

	// Make E and B point to cell [0]
	emf->E = emf->E_buf + gc[0];
	emf->B = emf->B_buf + gc[0];

	// Set cell sizes and box limits
	emf -> box = box;
	emf -> dx = box / nx;

	// Set time step
	emf -> dt = dt;

	// Reset iteration number
	emf -> iter = 0;

	// Reset moving window information
	emf -> moving_window = 0;
	emf -> n_move = 0;

	// Default to periodic boundary condtions
	emf -> bc_type = EMF_BC_PERIODIC;

	emf -> mur_fld[0].x = emf -> mur_fld[0].y = emf -> mur_fld[0].z = 0;
	emf -> mur_fld[1].x = emf -> mur_fld[1].y = emf -> mur_fld[1].z = 0;

	emf -> mur_tmp[0].x = emf -> mur_tmp[0].y = emf -> mur_tmp[0].z = 0;
	emf -> mur_tmp[1].x = emf -> mur_tmp[1].y = emf -> mur_tmp[1].z = 0;

	// Disable external fields by default
	emf -> ext_fld.E_type = EMF_FLD_TYPE_NONE;
	emf -> ext_fld.B_type = EMF_FLD_TYPE_NONE;
	emf -> E_part = emf->E;
	emf -> B_part = emf->B;
}

void emf_delete( t_emf *emf )
{
	free( emf->E_buf );
	free( emf->B_buf );

	emf->E_buf = NULL;
	emf->B_buf = NULL;

	if ( emf -> ext_fld.E_type > EMF_FLD_TYPE_NONE ) {
		free( emf -> ext_fld.E_part_buf );
	}

	if ( emf -> ext_fld.B_type > EMF_FLD_TYPE_NONE ) {
		free( emf -> ext_fld.B_part_buf );
	}

	emf->E_part = NULL;
	emf->B_part = NULL;

}

/*********************************************************************************************

 Laser Pulses

*********************************************************************************************/

t_fld lon_env( const t_emf_laser* const laser, const t_fld z )
{

	if ( z > laser -> start ) {
		// Ahead of laser
		return 0.0;
	} else if ( z > laser -> start - laser -> rise ) {
		// Laser rise
		t_fld csi = z - laser -> start;
		t_fld e = sin( M_PI_2 * csi / laser->rise );
		return e*e;
	} else if ( z > laser -> start - (laser -> rise + laser -> flat) ) {
		// Flat-top
		return 1.0;
	} else if ( z > laser -> start - (laser -> rise + laser -> flat + laser -> fall) ) {
		// Laser fall
		t_fld csi = z - (laser -> start - laser -> rise - laser -> flat - laser -> fall);
		t_fld e = sin( M_PI_2 * csi / laser->fall );
		return e*e;
	}

	// Before laser
	return 0.0;
}

void emf_add_laser( t_emf* const emf, t_emf_laser* laser )
{
	// Validate laser parameters
	if ( laser -> fwhm != 0 ) {
		if ( laser -> fwhm <= 0 ) {
			fprintf(stderr, "Invalid laser FWHM, must be > 0, aborting.\n" );
			exit(-1);
		}

		// The fwhm parameter overrides the rise/flat/fall parameters
		laser -> rise = laser -> fwhm;
		laser -> fall = laser -> fwhm;
		laser -> flat = 0.;
	}

	if ( laser -> rise <= 0 ) {
		fprintf(stderr, "Invalid laser RISE, must be > 0, aborting.\n" );
		exit(-1);
	}

	if ( laser -> flat < 0 ) {
		fprintf(stderr, "Invalid laser FLAT, must be >= 0, aborting.\n" );
		exit(-1);
	}

	if ( laser -> fall <= 0 ) {
		fprintf(stderr, "Invalid laser FALL, must be > 0, aborting.\n" );
		exit(-1);
	}

	// Launch laser

	t_fld z, z_2;
	t_fld amp, lenv, lenv_2, k;
	t_fld dx;
	t_fld cos_pol, sin_pol;

	t_vfld* restrict E = emf -> E;
	t_vfld* restrict B = emf -> B;

	dx = emf -> dx;

	amp = laser -> omega0 * laser -> a0;

	cos_pol = cos( laser -> polarization );
	sin_pol = sin( laser -> polarization );

	k = laser -> omega0;

	for (int i = 0; i < emf->nx; i++) {
		z = i * dx;
		z_2 = z + dx/2;

		lenv   = amp * lon_env( laser, z );
		lenv_2 = amp * lon_env( laser, z_2 );

		// E[i + j*nrow].x += 0.0
		E[i].y += +lenv * cos( k * z ) * cos_pol;
		E[i].z += +lenv * cos( k * z ) * sin_pol;

		// E[i + j*nrow].x += 0.0
		B[i].y += -lenv_2 * cos( k * z_2 ) * sin_pol;
		B[i].z += +lenv_2 * cos( k * z_2 ) * cos_pol;

	}

	// Set guard cell values for periodic boundaries
	if ( emf -> bc_type == EMF_BC_PERIODIC ) emf_update_gc( emf );

}

/*********************************************************************************************

 Diagnostics

 *********************************************************************************************/

void emf_report( const t_emf *emf, const char field, const char fc )
{
	int i;
	char vfname[16];

	// Choose field to save
	t_vfld * restrict f;
	switch (field) {
		case EFLD:
			f = emf->E;
            snprintf(vfname,16,"E%1d",fc);
			break;
		case BFLD:
			f = emf->B;
            snprintf(vfname,16,"B%1d",fc);
			break;
		case EPART:
			f = emf->E_part;
            snprintf(vfname,16,"E%1d-part",fc);
			break;
		case BPART:
			f = emf->B_part;
            snprintf(vfname,16,"B%1d-part",fc);
			break;
		default:
			fprintf(stderr, "Invalid field type selected, returning\n");
			return;
	}

	// Pack the information
	float buf[ emf->nx ];
	switch (fc) {
		case 0:
			for ( i = 0; i < emf->nx; i++ ) {
				buf[i] = f[i].x;
			}
			break;
		case 1:
			for ( i = 0; i < emf->nx; i++ ) {
				buf[i] = f[i].y;
			}
			break;
		case 2:
			for ( i = 0; i < emf->nx; i++ ) {
				buf[i] = f[i].z;
			}
			break;
	}

    t_zdf_grid_axis axis[1];
    axis[0] = (t_zdf_grid_axis) {
    	.min = 0.0 + emf->n_move * emf->dx,
    	.max = emf->box + emf->n_move * emf->dx,
    	.label = "x_1",
    	.units = "c/\\omega_p"
    };

    t_zdf_grid_info info = {
    	.ndims = 1,
    	.label = vfname,
    	.units = "m_e c \\omega_p e^{-1}",
    	.axis = axis
    };

    info.nx[0] = emf->nx;

    t_zdf_iteration iter = {
    	.n = emf->iter,
    	.t = emf -> iter * emf -> dt,
    	.time_units = "1/\\omega_p"
    };

	zdf_save_grid( buf, &info, &iter, "EMF" );

}

/*********************************************************************************************

 Absorbing boundaries
 1st order MUR absorbing boundary conditions

 *********************************************************************************************/

void mur_abc( t_emf *emf ) {

    const int nx = emf->nx;
    float const S = (emf->dt - emf->dx) / (emf->dt + emf->dx);

	if ( emf -> bc_type == EMF_BC_OPEN) {
		// lower boundary
        emf -> mur_fld[0].y = emf -> mur_tmp[0].y + S * (emf -> E[0].y - emf -> mur_fld[0].y);
        emf -> mur_fld[0].z = emf -> mur_tmp[0].z + S * (emf -> E[0].z - emf -> mur_fld[0].z);

        emf ->  E[-1].y = emf -> mur_fld[0].y;
        emf ->  E[-1].z = emf -> mur_fld[0].z;

        // Store Eperp for next iteration
        emf -> mur_tmp[0].y = emf -> E[0].y;
        emf -> mur_tmp[0].z = emf -> E[0].z;

		// upper boundary
        emf -> mur_fld[1].y = emf -> mur_tmp[1].y + S * (emf -> E[nx-1].y - emf -> mur_fld[1].y);
        emf -> mur_fld[1].z = emf -> mur_tmp[1].z + S * (emf -> E[nx-1].z - emf -> mur_fld[1].z);

        emf ->  E[nx].y = emf -> mur_fld[1].y;
        emf ->  E[nx].z = emf -> mur_fld[1].z;

        // Store Eperp for next iteration
        emf -> mur_tmp[1].y = emf -> E[nx-1].y;
        emf -> mur_tmp[1].z = emf -> E[nx-1].z;
	}

}

/*********************************************************************************************

 Field solver

 *********************************************************************************************/

void yee_b( t_emf *emf, const float dt )
{
	// this must not be unsigned because we access negative cell indexes
	int i;
	t_fld dt_dx;

    t_vfld* const restrict B = emf -> B;
    const t_vfld* const restrict E = emf -> E;

	dt_dx = dt / emf->dx;

	// Canonical implementation
	for (i=-1; i<=emf->nx; i++) {
		// B[ i ].x += 0;  // Bx does not evolve in 1D
		B[ i ].y += (   dt_dx * ( E[i+1].z - E[ i ].z) );

		B[ i ].z += ( - dt_dx * ( E[i+1].y - E[ i ].y) );

	}
}

void yee_e( t_emf *emf, const t_current *current, const float dt )
{
	// this must not be unsigned because we access negative cell indexes
	int i;
	t_fld dt_dx;

	dt_dx = dt / emf->dx;

    t_vfld* const restrict E = emf -> E;
    const t_vfld* const restrict B = emf -> B;
    const t_vfld* const restrict J = current -> J;
    const int nx = emf->nx;

	// Canonical implementation

	for (i=0; i<=nx+1; i++) {
		E[i].x += (                                - dt * J[i].x );

		E[i].y += ( - dt_dx * ( B[i].z - B[i-1].z) - dt * J[i].y );

		E[i].z += ( + dt_dx * ( B[i].y - B[i-1].y) - dt * J[i].z );

	}

}

// This code operates with periodic boundaries
void emf_update_gc( t_emf *emf )
{
	int i;

    t_vfld* const restrict E = emf -> E;
    t_vfld* const restrict B = emf -> B;
    const int nx = emf->nx;

	if ( emf -> bc_type == EMF_BC_PERIODIC ) {
		// x

		// lower
		for (i=-emf->gc[0]; i<0; i++) {
			E[ i ].x = E[ nx + i ].x;
			E[ i ].y = E[ nx + i ].y;
			E[ i ].z = E[ nx + i ].z;

			B[ i ].x = B[ nx + i ].x;
			B[ i ].y = B[ nx + i ].y;
			B[ i ].z = B[ nx + i ].z;
		}

		// upper
		for (i=0; i<emf->gc[0]; i++) {
			E[ nx + i ].x = E[ i ].x;
			E[ nx + i ].y = E[ i ].y;
			E[ nx + i ].z = E[ i ].z;

			B[ nx + i ].x = B[ i ].x;
			B[ nx + i ].y = B[ i ].y;
			B[ nx + i ].z = B[ i ].z;
		}
	}

}

void emf_move_window( t_emf *emf ){

	if ( ( emf -> iter * emf -> dt ) > emf->dx*( emf -> n_move + 1 ) ) {
		int i;

	    t_vfld* const restrict E = emf -> E;
	    t_vfld* const restrict B = emf -> B;

	    const t_vfld zero_fld = {0.,0.,0.};

		// Shift data left 1 cell and zero rightmost cells

		for (i = -emf->gc[0]; i < emf->nx+emf->gc[1] - 1; i++) {
			E[ i ] = E[ i + 1 ];
			B[ i ] = B[ i + 1 ];
		}

		for( i = emf->nx - 1; i < emf->nx+emf->gc[1]; i ++) {
			E[ i ] = zero_fld;
			B[ i ] = zero_fld;
		}

		// Increase moving window counter
		emf -> n_move++;

	}

}

void emf_advance( t_emf *emf, const t_current *current )
{
	uint64_t t0 = timer_ticks();
	const float dt = emf->dt;

	// Advance EM field using Yee algorithm modified for having E and B time centered
	yee_b( emf, dt/2.0f );

	yee_e( emf, current, dt );

    // Process open boundaries if needed
    if ( emf->bc_type == EMF_BC_OPEN ) mur_abc( emf );

	yee_b( emf, dt/2.0f );

	// Update guard cells
	emf_update_gc( emf );

	// Update contribuition of external fields if necessary
	emf_update_part_fld( emf );

	// Advance internal iteration number
    emf -> iter += 1;

    // Move simulation window if needed
    if ( emf -> moving_window ) emf_move_window( emf );

    // Update timing information
	_emf_time += timer_interval_seconds(t0, timer_ticks());
}

void emf_get_energy( const t_emf *emf, double energy[] )
{
	int i;
    t_vfld* const restrict E = emf -> E;
    t_vfld* const restrict B = emf -> B;

	for( i = 0; i<6; i++) energy[i] = 0;

	for( i = 0; i < emf -> nx; i ++ ) {
		energy[0] += E[i].x * E[i].x;
		energy[1] += E[i].y * E[i].y;
		energy[2] += E[i].z * E[i].z;
		energy[3] += B[i].x * B[i].x;
		energy[4] += B[i].y * B[i].y;
		energy[5] += B[i].z * B[i].z;
	}

	for( i = 0; i<6; i++) energy[i] *= 0.5 * emf -> dx;

}

/*********************************************************************************************

External Fields

 *********************************************************************************************/

/**
 * Sets the external fields to be used for the simulation
 * @param   emf     EM field object
 * @param   ext_fld External fields
 */
void emf_set_ext_fld( t_emf* const emf, t_emf_ext_fld* ext_fld ) {

	emf -> ext_fld.E_type = ext_fld -> E_type;

	if ( emf -> ext_fld.E_type == EMF_FLD_TYPE_NONE ) {
		// Particle fields just point to the self-consistent fields
		emf -> E_part = emf -> E;
		emf -> ext_fld.E_part_buf = NULL;
	} else {
	    switch( emf -> ext_fld.E_type ) {
	        case( EMF_FLD_TYPE_UNIFORM ):
	        	emf -> ext_fld.E_0 = ext_fld->E_0;
	        	break;

	        case( EMF_FLD_TYPE_CUSTOM ):
	        	emf -> ext_fld.E_custom = ext_fld->E_custom;
	        	emf -> ext_fld.E_custom_data = ext_fld->E_custom_data;
	        	break;

	    	default:
	    		fprintf(stderr, "Invalid external field type, aborting.\n" );
				exit(-1);
	    }

		// Allocate space for additional field grids
        size_t size = (emf->gc[0] + emf->nx + emf->gc[1]) * sizeof( t_vfld ) ;
	
		emf->ext_fld.E_part_buf = malloc( size );
        emf->E_part = emf->ext_fld.E_part_buf + emf->gc[0];
	}

	emf -> ext_fld.B_type = ext_fld -> B_type;

	if ( emf -> ext_fld.B_type == EMF_FLD_TYPE_NONE ) {
		// Particle fields just point to the self-consistent fields
		emf -> B_part = emf -> B;
		emf -> ext_fld.B_part_buf = NULL;
	} else {
	    switch( emf -> ext_fld.B_type ) {
	        case( EMF_FLD_TYPE_UNIFORM ):
	        	emf -> ext_fld.B_0 = ext_fld->B_0;
	        	break;

	        case( EMF_FLD_TYPE_CUSTOM ):
	        	emf -> ext_fld.B_custom = ext_fld->B_custom;
	        	emf -> ext_fld.B_custom_data = ext_fld->B_custom_data;
	        	break;

	    	default:
	    		fprintf(stderr, "Invalid external field type, aborting.\n" );
				exit(-1);
	    }

		// Allocate space for additional field grids
        size_t size = (emf->gc[0] + emf->nx + emf->gc[1]) * sizeof( t_vfld ) ;
	
		emf->ext_fld.B_part_buf = malloc( size );
        emf->B_part = emf->ext_fld.B_part_buf + emf->gc[0];
	}

    // Initialize values on E/B_part grids
    emf_update_part_fld( emf );

}

/**
 * Updates field values seen by particles with externally imposed fields
 * @param emf EMF object holding field data
 */
void emf_update_part_fld( t_emf* const emf ) {

    // Restrict pointers to E_part
    t_vfld* const restrict E_part = emf->E_part;

    switch (emf->ext_fld.E_type)
    {
    case EMF_FLD_TYPE_UNIFORM: {
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            t_vfld e = emf -> E[i];
            e.x += emf->ext_fld.E_0.x;
            e.y += emf->ext_fld.E_0.y;
            e.z += emf->ext_fld.E_0.z;
            E_part[i] = e;
        }
        break; }
    case EMF_FLD_TYPE_CUSTOM: {
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            t_vfld ext_E = (*emf->ext_fld.E_custom)(i,emf->dx,emf->ext_fld.E_custom_data);

            t_vfld e = emf -> E[i];
            e.x += ext_E.x;
            e.y += ext_E.y;
            e.z += ext_E.z;
            E_part[i] = e;
        }
        break; }
    case EMF_FLD_TYPE_NONE:
        break;
    }

    // Restrict pointers to B_part
    t_vfld* const restrict B_part = emf->B_part;

    switch (emf->ext_fld.B_type)
    {
    case EMF_FLD_TYPE_UNIFORM: {
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            t_vfld b = emf -> B[i];
            b.x += emf->ext_fld.B_0.x;
            b.y += emf->ext_fld.B_0.y;
            b.z += emf->ext_fld.B_0.z;
            B_part[i] = b;
        }

    }
        break; 
    case EMF_FLD_TYPE_CUSTOM: {
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            t_vfld ext_B = (*emf->ext_fld.B_custom)(i,emf->dx,emf->ext_fld.B_custom_data);

            t_vfld b = emf -> B[i];
            b.x += ext_B.x;
            b.y += ext_B.y;
            b.z += ext_B.z;
            B_part[i] = b;
        }
    }
        break; 
    case EMF_FLD_TYPE_NONE:
        break;
    }

}

/**
 * Initialize EMF field values
 * @param emf       EM field object
 * @param init_fld  Initial field parameters
 */
void emf_init_fld( t_emf* const emf, t_emf_init_fld* init_fld )
{
    if ( emf -> iter != 0 ) {
        fprintf(stderr, "emf_init_fld should only be called at initialization, aborting...\n" );
        exit(-1);
    }

    t_vfld* const restrict E = emf->E;
    t_vfld* const restrict B = emf->B;

    switch ( init_fld -> E_type )
    {
    case EMF_FLD_TYPE_NONE:
        break;

    case EMF_FLD_TYPE_UNIFORM:
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            E[ i ] = init_fld -> E_0;
        }
        break;

    case EMF_FLD_TYPE_CUSTOM:
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            t_vfld init_E = (init_fld->E_custom)
                (i,emf->dx, init_fld->E_custom_data);
            E[ i ] = init_E;
        }
        break;
    }    

    switch ( init_fld -> B_type )
    {
    case EMF_FLD_TYPE_NONE:
        break;

    case EMF_FLD_TYPE_UNIFORM:
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            B[ i ] = init_fld -> B_0;
        }
        break;

    case EMF_FLD_TYPE_CUSTOM:
        for (int i=-emf->gc[0]; i<emf->nx+emf->gc[1]; i++) {
            t_vfld init_B = (init_fld->B_custom)
                (i,emf->dx, init_fld->B_custom_data);
            B[ i ] = init_B;
        }
        break;
    }    

}

