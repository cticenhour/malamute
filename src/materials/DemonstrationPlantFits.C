/****************************************************************************/
/*                        DO NOT MODIFY THIS HEADER                         */
/*                                                                          */
/* MALAMUTE: MOOSE Application Library for Advanced Manufacturing UTilitiEs */
/*                                                                          */
/*           Copyright 2021 - 2024, Battelle Energy Alliance, LLC           */
/*                           ALL RIGHTS RESERVED                            */
/****************************************************************************/

#include "DemonstrationPlantFits.h"

registerMooseObject("MalamuteApp", DemonstrationPlantFits);

InputParameters
DemonstrationPlantFits::validParams()
{
  InputParameters params = ADMaterial::validParams();
  params.addClassDescription(
      "Material properties corresponding to demonstration reference material.");
  params.addParam<Real>("c_mu0", 0.15616, "mu0 coefficient");
  params.addParam<Real>("c_mu1", -3.3696e-5, "mu1 coefficient");
  params.addParam<Real>("c_mu2", 1.0191e-8, "mu2 coefficient");
  params.addParam<Real>("c_mu3", -1.0413e-12, "mu3 coefficient");
  params.addParam<Real>("Tmax", 4000, "The maximum temperature");
  params.addParam<Real>("Tl", 1623, "The liquidus temperature");
  params.addParam<Real>(
      "T90", 1528, "The T90 temperature (I don't know what this means physically)");
  params.addParam<Real>("beta", 1e11, "beta coefficient");
  params.addParam<Real>("c_k0", 10.7143, "k0 coefficient");
  params.addParam<Real>("c_k1", 14.2857e-3, "k0 coefficient");
  params.addParam<Real>("c_cp0", 425.75, "cp0 coefficient");
  params.addParam<Real>("c_cp1", 170.833e-3, "cp1 coefficient");
  params.addParam<Real>("c_rho0", 7.9e3, "The constant density");
  params.addCoupledVar("temperature", 1., "The temperature");
  params.addParam<MaterialPropertyName>(
      "mu_name", "mu", "The name of the viscosity material property");
  params.addParam<MaterialPropertyName>("k_name", "k", "The name of the thermal conductivity");
  params.addParam<MaterialPropertyName>("cp_name", "cp", "The name of the thermal conductivity");
  params.addParam<MaterialPropertyName>("rho_name", "rho", "The name of the thermal conductivity");
  params.addParam<int>("length_unit_exponent",
                       0,
                       "The exponent of the length unit. If working in milimeters for example, "
                       "this number should be -3");
  params.addParam<int>(
      "temperature_unit_exponent",
      0,
      "The exponent of the temperature unit. If working in kili-Kelvin for example, "
      "this number should be 3");
  params.addParam<int>("mass_unit_exponent",
                       0,
                       "The exponent of the mass unit. If working in miligrams for example, "
                       "this number should be -9");
  params.addParam<int>("time_unit_exponent",
                       0,
                       "The exponent of the time unit. If working in micro-seconds for example, "
                       "this number should be -6");
  return params;
}

DemonstrationPlantFits::DemonstrationPlantFits(const InputParameters & parameters)
  : ADMaterial(parameters),
    _c_mu0(getParam<Real>("c_mu0")),
    _c_mu1(getParam<Real>("c_mu1")),
    _c_mu2(getParam<Real>("c_mu2")),
    _c_mu3(getParam<Real>("c_mu3")),
    _Tmax(getParam<Real>("Tmax")),
    _Tl(getParam<Real>("Tl")),
    _T90(getParam<Real>("T90")),
    _beta(getParam<Real>("beta")),
    _c_k0(getParam<Real>("c_k0")),
    _c_k1(getParam<Real>("c_k1")),
    _c_cp0(getParam<Real>("c_cp0")),
    _c_cp1(getParam<Real>("c_cp1")),
    _c_rho0(getParam<Real>("c_rho0")),
    _temperature(adCoupledValue("temperature")),
    _grad_temperature(adCoupledGradient("temperature")),
    _mu(declareADProperty<Real>(getParam<MaterialPropertyName>("mu_name"))),
    _k(declareADProperty<Real>(getParam<MaterialPropertyName>("k_name"))),
    _cp(declareADProperty<Real>(getParam<MaterialPropertyName>("cp_name"))),
    _rho(declareADProperty<Real>(getParam<MaterialPropertyName>("rho_name"))),
    _grad_k(declareADProperty<RealVectorValue>("grad_" + getParam<MaterialPropertyName>("k_name"))),
    _length_units_per_meter(1. / std::pow(10, getParam<int>("length_unit_exponent"))),
    _temperature_units_per_kelvin(1. / std::pow(10, getParam<int>("temperature_unit_exponent"))),
    _mass_units_per_kilogram(1. / std::pow(10, getParam<int>("mass_unit_exponent"))),
    _time_units_per_second(1. / std::pow(10, getParam<int>("time_unit_exponent")))
{
}

void
DemonstrationPlantFits::computeQpProperties()
{
  if (_temperature[_qp] < _Tl * _temperature_units_per_kelvin)
    _mu[_qp] = _mass_units_per_kilogram / (_length_units_per_meter * _time_units_per_second) *
               (_c_mu0 + _c_mu1 * _Tl + _c_mu2 * _Tl * _Tl + _c_mu3 * _Tl * _Tl * _Tl) *
               (_beta + (1 - _beta) * (_temperature[_qp] - _T90 * _temperature_units_per_kelvin) /
                            ((_Tl - _T90) * _temperature_units_per_kelvin));
  else
  {
    ADReal That;
    That = _temperature[_qp] / _temperature_units_per_kelvin > _Tmax
               ? _Tmax
               : _temperature[_qp] / _temperature_units_per_kelvin;
    _mu[_qp] = _mass_units_per_kilogram / (_length_units_per_meter * _time_units_per_second) *
               (_c_mu0 + _c_mu1 * That + _c_mu2 * That * That + _c_mu3 * That * That * That);
  }
  _k[_qp] = (_c_k0 + _c_k1 / _temperature_units_per_kelvin * _temperature[_qp]) *
            (_mass_units_per_kilogram * _length_units_per_meter /
             (_temperature_units_per_kelvin * _time_units_per_second * _time_units_per_second *
              _time_units_per_second));
  _grad_k[_qp] = _c_k1 *
                 (_mass_units_per_kilogram * _length_units_per_meter /
                  (_temperature_units_per_kelvin * _temperature_units_per_kelvin *
                   _time_units_per_second * _time_units_per_second * _time_units_per_second)) *
                 _grad_temperature[_qp];
  _cp[_qp] = (_c_cp0 + _c_cp1 / _temperature_units_per_kelvin * _temperature[_qp]) *
             (_length_units_per_meter * _length_units_per_meter /
              (_temperature_units_per_kelvin * _time_units_per_second * _time_units_per_second));
  _rho[_qp] = _c_rho0 * _mass_units_per_kilogram /
              (_length_units_per_meter * _length_units_per_meter * _length_units_per_meter);
}
