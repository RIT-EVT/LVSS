clc; close all; clear;

% Parameters %
IoutMax = 18; % 18 A
Ksns = 2000; % Current Sense Ratio
adcVoltage = 3.3;
Kcl = 140; % Current Limit Ratio

% Current Limit Calculations %
Vsns = 1000 * 3 / Ksns;
Rsns = adcVoltage * Ksns / IoutMax;
Rilim = Kcl / IoutMax;

% Compute Device Temperature based on the equation (I_SNST mA - 0.85) / (dI_SNST/dT) mA/°C  + 25°C on page 34 of TPS2HB35-Q1 datasheet %
Isnst = 0.85;
refTemperature = 25; % Reference temperature in degrees celsius
refCurrent = 0.85; % Reference current in mA for reference temperature

tj = (Isnst - refCurrent) / 0.011 + refTemperature;
fprintf("Device temperature at %0.4f mA is %0.4f°C\n", Isnst, tj);
temperatureTable = table(Isnst, tj, 'VariableNames', {'Current_mA', 'Temperature_C'});

% ADC Data %
testVals = (1:10) * 1e-3; % 1mV to 10mV
adcData = ((330 * testVals) * 4096) / adcVoltage; % Convert to ADC counts

adcTable = array2table([testVals(:), adcData(:)], ...
    "VariableNames", {'Input (mA)', 'ADC Counts'});

% Table with all major data %
FinalTable = array2table([Vsns; IoutMax; Rsns; Ksns; Rilim; min(adcData); max(adcData)], ...
    'RowNames', ["Vsns (V)"; "IoutMax (A)"; "Rsns (Ω)"; "Ksns"; "Rilim (kΩ)"; "Min ADC Counts"; "Max ADC Counts"], ...
    'VariableNames', {'Value'});