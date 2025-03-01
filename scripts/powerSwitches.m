clc; close all; clear;

% Parameters %
IoutMax = 18; % 18 A
Ksns =  2000; % Current Sense Ratio
adcVoltage = 3.3;

% Vsns %
Vsns = 1000 * 3 / Ksns;

% Rsns %
Rsns = adcVoltage * Ksns / IoutMax;

% Rilim
Kcl = 140;
Icl = 18;
Rilim = Kcl / Icl;

% Define table
columnNames = ["Vsns (V)", "IoutMax (A)", "Rsns (Ω)", "Ksns", "Rilim (kΩ)"];
data = [Vsns, IoutMax, Rsns, Ksns, Rilim];
T = array2table(data, 'VariableNames', columnNames);
disp(T)

% Based on the equation (I_SNST mA - 0.85) / (dI_SNST /dT) mA/°C  + 25°C on page 34 of TPS2HB35-Q1 datasheet
Isnst = 0.12;
tj = (Isnst - 0.85) / 0.011 + 25;
fprintf("Device temperature at %0.2f mA = %0.4f%cC\n\n", Isnst, tj, char(176))