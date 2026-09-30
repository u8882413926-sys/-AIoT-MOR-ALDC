@echo off
title AIoT-MOR-ALDC EK-RA8P1 Data Feeder
cls
echo =====================================================================
echo           AIoT-MOR-ALDC : SENSOR DATA FEEDER FOR EK-RA8P1
echo =====================================================================
echo.
echo Choose an option:
echo   [1] Stream Curated Intel Dataset (sensor_data.txt, 550 samples @ 1 Hz)
echo   [2] Stream Full Intel Dataset (intel_windows_full.txt, 34,400 samples @ 1 Hz)
echo   [3] Fast Stream Intel Dataset (intel_windows_full.txt @ 10 Hz)
echo   [4] Stream directly from Downloads CSV (intel_windows.csv)
echo   [5] Open sensor_data.txt in Windows Notepad to view / edit
echo   [6] Start Interactive Command Line (type values, inject anomalies)
echo   [7] Re-convert / update dataset from intel_windows.csv
echo   [8] Exit
echo.
set /p opt="Enter choice [1-8] and press Enter: "

if "%opt%"=="1" (
    echo.
    echo Streaming Curated Intel Thermal Dataset to EK-RA8P1 (1 Hz)...
    python laptop_sensor_feeder.py -f sensor_data.txt --rate 1.0
    pause
    goto end
)

if "%opt%"=="2" (
    echo.
    echo Streaming Full Intel Thermal Dataset (34,400 readings) at 1 Hz...
    python laptop_sensor_feeder.py -f intel_windows_full.txt --rate 1.0
    pause
    goto end
)

if "%opt%"=="3" (
    echo.
    echo Fast Streaming Full Intel Thermal Dataset at 10 Hz (10 samples/sec)...
    python laptop_sensor_feeder.py -f intel_windows_full.txt --rate 0.1
    pause
    goto end
)

if "%opt%"=="4" (
    echo.
    echo Streaming directly from C:\Users\udaya\Downloads\intel_windows.csv...
    python laptop_sensor_feeder.py -f "C:\Users\udaya\Downloads\intel_windows.csv" --rate 1.0
    pause
    goto end
)

if "%opt%"=="5" (
    notepad sensor_data.txt
    goto end
)

if "%opt%"=="6" (
    python laptop_sensor_feeder.py
    pause
    goto end
)

if "%opt%"=="7" (
    echo.
    echo Running convert_intel_csv.py...
    python convert_intel_csv.py
    pause
    goto end
)

:end
