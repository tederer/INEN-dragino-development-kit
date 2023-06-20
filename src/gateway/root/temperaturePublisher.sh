#!/bin/sh

SERVER_HOST=provider
SERVER_PORT=8100
SENSOR_ID=arduino
INTERVAL_IN_SEC=5

INPUT_FILE=/var/iot/data
MAX_HTTP_POST_DURATION_IN_SEC=2

while [ 1 ]; do
   timestamp=$(date)
   inputFileContent=$(cat $INPUT_FILE 2> /dev/null | tr -d '\r\n')
   exitCode=$?
   if [ $exitCode -ne 0 ]; then
      continue
   fi
   rm -f $INPUT_FILE
   if [ ${#inputFileContent} -le 0 ]; then
      continue
   fi
   
   sensorId=$(echo $inputFileContent | awk -F "," '/'$SENSOR_ID'/ {print $1}')
   sensorValue=$(echo $inputFileContent | awk -F "," '/'$SENSOR_ID'/ {print $2}')

   curl -X POST --max-time $MAX_HTTP_POST_DURATION_IN_SEC \
                -H "Content-Type: application/json" \
                -d "{\"sensorId\":\"$sensorId\",\"temperature\":$sensorValue}" \
                http://$SERVER_HOST:$SERVER_PORT/sensordata
   
   sleep $INTERVAL_IN_SEC
done
