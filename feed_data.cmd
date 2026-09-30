@echo off
title AIoT-MOR-ALDC EK-RA8P1 Data Feeder
cls
echo =====================================================================
echo           AIoT-MOR-ALDC : SENSOR DATA FEEDER FOR EK-RA8P1
echo =====================================================================
echo.
echo Choose an option:
echo   [1] Stream sensor_data.txt to EK-RA8P1 board (1 sample/sec)
echo   [2] Open sensor_data.txt in Windows Notepad to edit values
echo   [3] Start interactive command line (type values like 35.5, 60.0)
echo   [4] Exit
echo.
set /p opt="Enter choice [1-4] and press Enter: "

if "%opt%"=="1" (
    echo.
    echo Streaming sensor_data.txt to EK-RA8P1...
    python laptop_sensor_feeder.py -f sensor_data.txt
    pause
    goto end
)

if "%opt%"=="2" (
    notepad sensor_data.txt
    goto end
)

if "%opt%"=="3" (
    python laptop_sensor_feeder.py
    pause
    goto end
)

:end
