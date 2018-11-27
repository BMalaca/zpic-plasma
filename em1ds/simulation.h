#ifndef __SIMULATION__
#define __SIMULATION__

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "filter.h"
#include "particles.h"
#include "emf.h"
#include "current.h"

typedef struct {

	// Time step
	float dt;
	float tmax;

	// Diagnostic frequency
	int ndump;

	// Simulation data
	int n_species;
	t_species* species;
	t_emf emf;
	t_current current;
	t_charge charge;

	// FFT
	t_fftr_cfg fft_forward,fft_backward;

	// Spectral filtering
	t_filter filter;

} t_simulation;


void sim_init( t_simulation* sim );
void sim_report( t_simulation* sim );

void sim_iter( t_simulation* sim );
void sim_report_energy( t_simulation* sim );

void sim_add_laser( t_simulation* sim,  t_emf_laser* laser );
void sim_add_neutral_bkg( t_simulation* sim );

int sim_filter_set( t_simulation* const sim, enum filter_type const type, float const ck );

void sim_new( t_simulation* sim, int nx, float box, float dt, float tmax, int ndump, t_species* species, int n_species );
int report( int n, int ndump );
void sim_timings( t_simulation* sim, uint64_t t0, uint64_t t1 );
void sim_delete( t_simulation* sim );

#endif
