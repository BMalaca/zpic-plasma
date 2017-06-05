/*
 *  grid2d.c
 *  zpic
 *
 *  Created by Ricardo Fonseca on 12/8/10.
 *  Copyright 2010 Centro de Física dos Plasmas. All rights reserved.
 *
 */

#include "grid2d.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************************
	Scalar grids
 ****************************************************************************************/

int scalar_grid2d_init( t_scalar_grid2d *grid, const unsigned int nx[], const unsigned int gc[][2] )
{

	// store nx and gc values
	grid->nx[0] = nx[0];
	grid->nx[1] = nx[1];

	if ( gc ) {
		grid->gc[0][0] = gc[0][0];
		grid->gc[0][1] = gc[0][1];
		grid->gc[1][0] = gc[1][0];
		grid->gc[1][1] = gc[1][1];
	} else {
		grid->gc[0][0] = grid->gc[0][1] = 0;
		grid->gc[1][0] = grid->gc[1][1] = 0;
	}


	size_t size = (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				  (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]);
	
	grid -> buffer = (float *) malloc( size * sizeof( float ) );
	
	if ( !grid -> buffer ) {
		fprintf(stderr, "(*error*) Unable to allocate memory for fld_grid variable\n");
		return(-1);
	}

	grid -> nrow = grid->gc[0][0] + grid->nx[0] + grid->gc[0][1];

	// Make s point to cell [0]
	grid -> s = grid -> buffer + grid->gc[0][0] + grid->gc[1][0] * grid -> nrow ;

	return 0;
}


int scalar_grid2d_cleanup( t_scalar_grid2d *grid )
{
	free( grid -> buffer );

	grid -> buffer = NULL;
	grid -> s = NULL;

	grid->nx[0] = grid->nx[1] = 0;
	grid->gc[0][0] = grid->gc[0][1] = 0;
	grid->gc[1][0] = grid->gc[1][1] = 0;

	return 0;
}

void scalar_grid2d_zero( t_scalar_grid2d *grid ) {
	size_t size = (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				  (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]) *
				  sizeof(float);
	memset( grid -> buffer, 0, size );
}

void scalar_grid2d_copy( t_scalar_grid2d * restrict dst, const t_scalar_grid2d * restrict src ){
	size_t size = (src->gc[0][0] + src->nx[0] + src->gc[0][1]) *
				  (src->gc[1][0] + src->nx[1] + src->gc[1][1]) *
				  sizeof(float);
	memcpy( dst->buffer, src->buffer, size);
}

void scalar_grid2d_add( t_scalar_grid2d *dst, const t_scalar_grid2d *src ){
	unsigned long size = (src->gc[0][0] + src->nx[0] + src->gc[0][1]) *
				         (src->gc[1][0] + src->nx[1] + src->gc[1][1]);

	unsigned long i;
	for( i = 0; i < size; i++) dst->buffer[i] += src->buffer[i];
}


/****************************************************************************************
	Complex Scalar grids
 ****************************************************************************************/

int cscalar_grid2d_init( t_cscalar_grid2d *grid, const unsigned int nx[], const unsigned int gc[][2] )
{

	// store nx and gc values
	grid->nx[0] = nx[0];
	grid->nx[1] = nx[1];

	if ( gc ) {
		grid->gc[0][0] = gc[0][0];
		grid->gc[0][1] = gc[0][1];
		grid->gc[1][0] = gc[1][0];
		grid->gc[1][1] = gc[1][1];
	} else {
		grid->gc[0][0] = grid->gc[0][1] = 0;
		grid->gc[1][0] = grid->gc[1][1] = 0;
	}


	size_t size = (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				  (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]);

	
	grid -> buffer = (float complex *) malloc( size * sizeof( float complex ) );
	
	if ( !grid -> buffer ) {
		fprintf(stderr, "(*error*) Unable to allocate memory for fld_grid variable\n");
		return(-1);
	}

	grid -> nrow = grid->gc[0][0] + grid->nx[0] + grid->gc[0][1];

	// Make s point to cell [0]
	grid -> s = grid -> buffer + grid->gc[0][0] + grid->gc[1][0] * grid -> nrow ;	

	return 0;
}


int cscalar_grid2d_cleanup( t_cscalar_grid2d *grid )
{
	free( grid -> buffer );

	grid -> buffer = NULL;
	grid -> s = NULL;

	grid->nx[0] = grid->nx[1] = 0;
	grid->gc[0][0] = grid->gc[0][1] = 0;
	grid->gc[1][0] = grid->gc[1][1] = 0;

	return 0;
}

void cscalar_grid2d_zero( t_cscalar_grid2d *grid ) {
	size_t size = (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				  (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]) * 
				  sizeof(float complex);
	memset( grid -> buffer, 0, size );
}

void cscalar_grid2d_add( t_cscalar_grid2d *dst, const t_cscalar_grid2d *src ){
	unsigned long size = (src->gc[0][0] + src->nx[0] + src->gc[0][1]) *
				         (src->gc[1][0] + src->nx[1] + src->gc[1][1]);

	unsigned long i;
	for( i = 0; i < size; i++) dst->buffer[i] += src->buffer[i];
}

/****************************************************************************************
	Vector field grids
 ****************************************************************************************/

int vfld_grid2d_init( t_vfld_grid2d *grid, const unsigned int nx[], const unsigned int gc[][2] )
{

	// store nx and gc values
	grid->nx[0] = nx[0];
	grid->nx[1] = nx[1];

	if ( gc ) {
		grid->gc[0][0] = gc[0][0];
		grid->gc[0][1] = gc[0][1];
		grid->gc[1][0] = gc[1][0];
		grid->gc[1][1] = gc[1][1];
	} else {
		grid->gc[0][0] = grid->gc[0][1] = 0;
		grid->gc[1][0] = grid->gc[1][1] = 0;
	}


	size_t size = (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				  (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]);
	
	grid -> buffer = malloc( 3 * size * sizeof( float ) );
	
	if ( !grid -> buffer ) {
		fprintf(stderr, "(*error*) Unable to allocate memory for fld_grid variable\n");
		return(-1);
	}

	grid -> nrow = grid->gc[0][0] + grid->nx[0] + grid->gc[0][1];
	
	// Make x, y and z point to cell [0]
	grid -> x = grid -> buffer + grid->gc[0][0] + grid->gc[1][0] * grid -> nrow;
	grid -> y = grid -> x + size;
	grid -> z = grid -> y + size;

	return 0;
}


int vfld_grid2d_cleanup( t_vfld_grid2d *grid )
{
	free( grid -> buffer );

	grid -> buffer = NULL;
	grid -> x = grid -> y = grid -> z = NULL;
	grid->nx[0] = grid->nx[1] = 0;
	grid->gc[0][0] = grid->gc[0][1] = 0;
	grid->gc[1][0] = grid->gc[1][1] = 0;

	return 0;
}

void vfld_grid2d_zero( t_vfld_grid2d *grid ) {
	size_t size = (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				  (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]) *
				  3 * sizeof(float);
	memset( grid -> buffer, 0, size );
}

void vfld_grid2d_add( t_vfld_grid2d * restrict dst, const t_vfld_grid2d * restrict src ){
	unsigned long size = (src->gc[0][0] + src->nx[0] + src->gc[0][1]) *
				         (src->gc[1][0] + src->nx[1] + src->gc[1][1]) * 3;

	unsigned long i;
	for( i = 0; i < size; i++) dst->buffer[i] += src->buffer[i];
}

/****************************************************************************************
	Complex Vector field grids
 ****************************************************************************************/

int cvfld_grid2d_init( t_cvfld_grid2d *grid, const unsigned int nx[], const unsigned int gc[][2] )
{

	// store nx and gc values
	grid->nx[0] = nx[0];
	grid->nx[1] = nx[1];

	if ( gc ) {
		grid->gc[0][0] = gc[0][0];
		grid->gc[0][1] = gc[0][1];
		grid->gc[1][0] = gc[1][0];
		grid->gc[1][1] = gc[1][1];
	} else {
		grid->gc[0][0] = grid->gc[0][1] = 0;
		grid->gc[1][0] = grid->gc[1][1] = 0;
	}


	size_t size = (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				  (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]);
	
	grid -> buffer = malloc( 3 * size * sizeof( float complex ) );
	
	if ( !grid -> buffer ) {
		fprintf(stderr, "(*error*) Unable to allocate memory for fld_grid variable\n");
		return(-1);
	}

	grid -> nrow = grid->gc[0][0] + grid->nx[0] + grid->gc[0][1];
	
	// Make x, y and z point to cell [0]
	grid -> x = grid -> buffer + grid->gc[0][0] + grid->gc[1][0] * grid -> nrow;
	grid -> y = grid -> x + size;
	grid -> z = grid -> y + size;

	return 0;
}


int cvfld_grid2d_cleanup( t_cvfld_grid2d *grid )
{
	free( grid -> buffer );

	grid -> buffer = NULL;
	grid -> x = grid -> y = grid -> z = NULL;

	grid->nx[0] = grid->nx[1] = 0;
	grid->gc[0][0] = grid->gc[0][1] = 0;
	grid->gc[1][0] = grid->gc[1][1] = 0;

	return 0;

}

void cvfld_grid2d_zero( t_cvfld_grid2d *grid ) {
	size_t size =  (grid->gc[0][0] + grid->nx[0] + grid->gc[0][1]) *
				   (grid->gc[1][0] + grid->nx[1] + grid->gc[1][1]) * 
				   3 * sizeof(float complex);
	memset( grid -> buffer, 0, size );
}

void cvfld_grid2d_add( t_cvfld_grid2d * restrict dst, const t_cvfld_grid2d * restrict src ){
	unsigned long size = (src->gc[0][0] + src->nx[0] + src->gc[0][1]) *
				         (src->gc[1][0] + src->nx[1] + src->gc[1][1]) * 3;

	unsigned long i;
	for( i = 0; i < size; i++) dst->buffer[i] += src->buffer[i];
}


