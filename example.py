import numpy as np
import sve
import logging

#--- Reading spectra ----------------------------------------------
logging.info("Reading spectra")
sun_spectrum = sve.read_spectrum("input/spectra/examples/narval_sun.s.gz")
mu_cas_a_spectrum = sve.read_spectrum("input/spectra/examples/narval_mu_cas.s.gz")


#--- Plotting (requires graphical interface) ----------------------------------------------
#logging.info("Plotting...")
#sve.plot_spectra([sun_spectrum, mu_cas_a_spectrum])
#sve.show_histogram(sun_spectrum['flux'])

# Stats
logging.info("Stats...")
print "Min & max wavelengths:", np.min(sun_spectrum['waveobs']), np.max(sun_spectrum['waveobs'])
print "Min & max fluxes:", np.min(sun_spectrum['flux']), np.max(sun_spectrum['flux'])


#--- Cut ----------------------------------------------
logging.info("Cutting...")
wfilter = np.logical_and(sun_spectrum['waveobs'] >= 480, sun_spectrum['waveobs'] <=670)
sun_spectrum = sun_spectrum[wfilter]

wfilter = np.logical_and(mu_cas_a_spectrum['waveobs'] >= 480, mu_cas_a_spectrum['waveobs'] <=670)
mu_cas_a_spectrum = mu_cas_a_spectrum[wfilter]


#--- Radial Velocity determination and correction ----------------------------------------------
logging.info("Radial velocity determination...")
# - Atomic
vald_linelist_file = "input/linelists/VALD/VALD.300_1100nm_teff_5770.0_logg_4.40.lst"
linelist_atomic = sve.read_VALD_linelist(vald_linelist_file, minimum_depth=0.0)

xcoord, fluxes, num_used_lines = sve.build_velocity_profile(mu_cas_a_spectrum, linelist_atomic, lower_velocity_limit=-200.0, upper_velocity_limit=200.0, velocity_step=1.0)

models = sve.modelize_velocity_profile(xcoord, fluxes)
good = sve.select_good_velocity_profile_models(models, xcoord, fluxes)
models = models[good]

# Number of models represent the number of components
spectroscopic_nary = str(len(models) >= 2)
rv = np.round(models[0].mu(), 2) # km/s

logging.info("Radial velocity correction...")
mu_cas_a_spectrum = sve.correct_velocity(mu_cas_a_spectrum, rv)


#--- Barycentric Velocity determination ----------------------------------------------
logging.info("Barycentric velocity determination...")
# - Telluric
telluric_linelist_file = "input/linelists/telluric/standard_atm_air_model.lst"
linelist_telluric = sve.read_telluric_linelist(telluric_linelist_file, minimum_depth=0.0)

xcoord, fluxes, num_used_lines = sve.build_velocity_profile(sun_spectrum, linelist_telluric, lower_velocity_limit=-100.0, upper_velocity_limit=100.0, velocity_step=0.5)

models = sve.modelize_velocity_profile(xcoord, fluxes, only_one_peak=True)
bv = np.round(models[0].mu(), 2) # km/s


#--- Resolution degradation ----------------------------------------------
logging.info("Resolution degradation...")
from_resolution = 80000
to_resolution = 40000
convolved_sun_spectrum = sve.convolve_spectrum(sun_spectrum, from_resolution, to_resolution=to_resolution)
convolved_mu_cas_a_spectrum = sve.convolve_spectrum(mu_cas_a_spectrum, from_resolution, to_resolution=to_resolution)


#--- Resampling and combining ----------------------------------------------
logging.info("Resampling and comibining...")
wavelengths = np.arange(480.0, 670.0, 0.001)
resampled_sun_spectrum = sve.resample_spectrum(sun_spectrum, wavelengths)
resampled_mu_cas_a_spectrum = sve.resample_spectrum(mu_cas_a_spectrum, wavelengths)

# Coadd previously resampled spectra
total_wavelengths = len(resampled_sun_spectrum)
coadded_spectrum = np.recarray((total_wavelengths, ), dtype=[('waveobs', float),('flux', float),('err', float)])
coadded_spectrum = resampled_sun_spectrum['waveobs']
coadded_spectrum = resampled_sun_spectrum['flux'] + resampled_mu_cas_a_spectrum['flux']
coadded_spectrum = np.sqrt(np.power(resampled_sun_spectrum['err'],2) + np.power(resampled_mu_cas_a_spectrum['err'],2))


##--- Continuum normalization ----------------------------------------------
logging.info("Continuum normalization...")
sun_continuum_model = sve.fit_continuum(sun_spectrum)
normalized_sun_spectrum = np.recarray((len(sun_spectrum), ), dtype=[('waveobs', float),('flux', float),('err', float)])
normalized_sun_spectrum['waveobs'] = sun_spectrum['waveobs']
normalized_sun_spectrum['flux'] = sun_spectrum['flux'] / sun_continuum_model(sun_spectrum['waveobs'])
normalized_sun_spectrum['err'] = sun_spectrum['err'] / sun_continuum_model(sun_spectrum['waveobs'])

# Or limit the fitting to given regions
continuum_regions = sve.read_continuum_regions("input/regions/continuum_regions.txt")
sun_continuum_model = sve.fit_continuum(sun_spectrum, segments=continuum_regions)


##--- Find continuum regions ----------------------------------------------
logging.info("Finding continuum regions...")
resolution = 80000
sigma = 0.001
max_continuum_diff = 1.0
fixed_wave_step = 0.05
sun_continuum_regions = sve.find_continuum(sun_spectrum, resolution, max_std_continuum = sigma, continuum_model = sun_continuum_model, max_continuum_diff=max_continuum_diff, fixed_wave_step=fixed_wave_step)
sve.write_continuum_regions(sun_continuum_regions, "sun_continuum_regions.txt")

# Or limit the search to given segments
segments = sve.read_segment_regions("input/regions/segments.txt")
limited_sun_continuum_regions = sve.find_continuum(sun_spectrum, resolution, segments=segments, max_std_continuum = sigma, continuum_model = sun_continuum_model, max_continuum_diff=max_continuum_diff, fixed_wave_step=fixed_wave_step)
sve.write_continuum_regions(limited_sun_continuum_regions, "limited_sun_continuum_regions.txt")

#--- Find linemasks ----------------------------------------------
logging.info("Finding line masks...")
sun_continuum_model = sve.fit_continuum(sun_spectrum)
vald_linelist_file = "input/linelists/VALD/VALD.300_1100nm_teff_5770.0_logg_4.40.lst"
chemical_elements_file = "input/abundances/chemical_elements_symbols.dat"
molecules_file = "input/abundances/molecular_symbols.dat"
telluric_linelist_file = "input/linelists/telluric/standard_atm_air_model.lst"
resolution = 80000
smoothed_sun_spectrum = sve.convolve_spectrum(sun_spectrum, resolution)
min_depth = 0.05
max_depth = 1.00
vel_atomic = 0.00  # km/s
vel_telluric = 17.79  # km/s
sun_linemasks = sve.find_linemasks(sun_spectrum, sun_continuum_model, vald_linelist_file, chemical_elements_file, molecules_file, telluric_linelist_file, minimum_depth=min_depth, maximum_depth=max_depth, smoothed_spectrum=smoothed_sun_spectrum, discard_gaussian=False, discard_voigt=True, vel_atomic=vel_atomic, vel_telluric=vel_telluric)
sve.write_line_regions(sun_linemasks, "sun_linemasks.txt")

##--- Barycentric velocity correction ----------------------------------------------
logging.info("Calculating barycentric velocity correction...")
day = 15
month = 2
year = 2012
hours = 0
minutes = 0
seconds = 0
ra_hours = 19
ra_minutes = 50
ra_seconds = 46.99
dec_degrees = 8
dec_minutes = 52
dec_seconds = 5.96

# Project velocity toward star
barycentric_vel = sve.calculate_barycentric_velocity_correction((year, month, day, hours, minutes, seconds), (ra_hours, ra_minutes, ra_seconds, dec_degrees, dec_minutes, dec_seconds))


##--- Estimate SNR from flux ----------------------------------------------
logging.info("Estimating SNR...")
num_points = 10
estimated_snr = sve.estimate_snr(sun_spectrum['flux'], num_points=num_points)


##--- Clean fluxes and errors ----------------------------------------------
logging.info("Cleaning fluxes and errors...")
flux_base = 0.0
flux_top = 1.0
err_base = 0.0
err_top = 1.0
ffilter = (sun_spectrum['flux'] > flux_base) & (sun_spectrum['flux'] <= flux_top)
efilter = (sun_spectrum['err'] > err_base) & (sun_spectrum['err'] <= err_top)
wfilter = np.logical_and(ffilter, efilter)
sun_spectrum = sun_spectrum[wfilter]


##--- Fit lines ----------------------------------------------
logging.info("Fitting lines...")
vald_linelist_file = "input/linelists/VALD/VALD.300_1100nm_teff_5770.0_logg_4.40.lst"
chemical_elements_file = "input/abundances/chemical_elements_symbols.dat"
molecules_file = "input/abundances/molecular_symbols.dat"
telluric_linelist_file = "input/linelists/telluric/standard_atm_air_model.lst"
vel_atomic = 0.00 # km/s
vel_telluric = 17.79 # km/s
line_regions = sve.read_line_regions("input/regions/line_masks.txt")
linemasks = sve.fit_lines(line_regions, sun_spectrum, sun_continuum_model, vel_atomic, vel_telluric, vald_linelist_file, chemical_elements_file, molecules_file, telluric_linelist_file, discard_gaussian=False, discard_voigt=True)


##--- Save spectrum ----------------------------------------------
logging.info("Saving spectrum...")
sve.write_spectrum(sun_spectrum, "sun.s", compress=False)
sve.write_spectrum(mu_cas_a_spectrum, "mu_cas_a.s", compress=False)
