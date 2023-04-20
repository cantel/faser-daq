python3 CheckConfigs.py \
--directory ../configs \
--schemas   \
schemas/DigitizerReceiver.schema \
schemas/DigitizerMonitor.schema     \
schemas/DigitizerNoiseMonitor.schema     \
schemas/DigitizerReceiver.schema     \
schemas/EmulatorMonitor.schema     \
schemas/EventBuilderFaser.schema     \
schemas/EventMonitor.schema     \
schemas/EventPlayback.schema     \
schemas/FileWriterFaser.schema     \
schemas/FrontEndEmulator.schema     \
schemas/FrontEndMonitor.schema     \
schemas/FrontEndReceiver.schema     \
schemas/SCTDataMonitor.schema     \
schemas/TrackStationMonitor.schema     \
schemas/TrackerReceiver.schema     \
schemas/TriggerGenerator.schema     \
schemas/TriggerMonitor.schema     \
schemas/TriggerRateMonitor.schema     \
schemas/TriggerReceiver.schema     \
--templates \
Templates/TLB.json \
Templates/TRB.json \
Templates/digitizer.json \
Templates/emulator.json \
Templates/emulatorIPC.json \
Templates/fileWriter.json \
Templates/monitor.json \
Templates/eventBuilder.json \
Templates/digitizerNoise.json \
--configs   \
digitizerSciLab.json \
digitizerTI12.json \
digiTLBSciLab.json \
tlbDigiTI12.json \
tlbDigiTI12LED.json \
tlbDigiTI12Cosmics.json \
tlbDigiTI12PMTs.json \
tlbDigiTI12PE.json \
tlbDigiTI12SinglePMTs.json \
tlbDigiTI12LEDLowerLowRange.json \
tlbDigiTI12LEDUpperLowRange.json \
combinedEHN1.json \
combinedEHN1_TLB_digit.json \
combinedSciLab.json \
combinedTI12Physics.json \
combinedTI12Cosmics.json \
combinedTI12PhysicsHighGain.json \
combinedTI12PhysicsLowGainLowRange.json \
playback.json              \
emulatorLocalhost.json \
emulatorLocalhostFullFarm.json \
emulatorLocalhostFullFarmIPC.json \
digitizerDarkRateSciLab.json \
digitizerLEDSciLab.json \
--extras   \
top.json \
monitor_top.json \
current.json \
grafana/faser_metrics.json                   \
schemas/validation-schema.json                   \
schemas/refs/connection-schema.json                   \
Templates/top.json                   \
Templates/monitor_top.json                   \
customized/host.json \
grafana.json \
fsm-rules.json \
demo-tree.json




