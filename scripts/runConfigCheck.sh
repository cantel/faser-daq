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
schemas/TrackerMonitor.schema     \
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
Templates/fileWriter.json \
Templates/monitor.json \
Templates/eventBuilder.json \
Templates/digitizerNoise.json \
--configs   \
digitizerSciLab.json \
digitizerTI12.json \
digiTLBSciLab.json \
tlbDigiTI12.json \
tlbDigiTI12Cosmics.json \
combinedEHN1.json \
combinedSciLab.json \
combinedTI12.json \
playback.json              \
emulatorLocalhost.json \
--extras   \
top.json \
current.json \
grafana/faser_metrics.json                   \
schemas/validation-schema.json                   \
Templates/top.json                   \
customized/host.json                 




