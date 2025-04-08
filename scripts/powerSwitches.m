clc;clear;close all;

% Parameters %
IoutMax = 18; % 18 A
Ksns = 2000; % Current Sense Ratio
adcVoltage = 3.3;
Kcl = 140; % Current Limit Ratio

% Current Limit Calculations %
Vsns = 1000 * 3 / Ksns; % Voltage going through the sns resistor
Rsns = adcVoltage * Ksns / IoutMax; % Value of the sns resistor
Rilim = Kcl / IoutMax; % Current limiting resistor

% Compute Device Temperature based on the equation (I_SNST mA - 0.85) / (dI_SNST/dT) mA/°C  + 25°C on page 34 of TPS2HB35-Q1 datasheet %
RsnsActual = 330; % Value of sns resistor on the LVSS
counts = 240; % Average ADC counts
mVolts = (counts * 3300) / 4096; % Voltage going through the ADC in milli volts
uAmps = mVolts * 1e3 / RsnsActual; % Current going through the ADC in micro amps
temperature = (uAmps - 0.85e3) / 0.011e3 + 25000; % Temperature in milli celsius

temperatureTable = array2table([counts; mVolts; uAmps; temperature], ...
    'RowNames', ["ADC Counts"; "Volts (mV)"; "Current (mA)"; "Temperature (mC)"], ...
    'VariableNames', {'Value'});

% Table with all major data %
FinalTable = array2table([Vsns; IoutMax; Rsns; Ksns; Rilim;], ...
    'RowNames', ["Vsns (V)"; "IoutMax (A)"; "Rsns (Ω)"; "Ksns"; "Rilim (kΩ)";], ...
    'VariableNames', {'Value'});