#!/bin/sh

SERVER_IP=10.130.1.205
SERVER_PORT=8080
SENSOR_ID=arduino

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
                -d "{\"id\":\"$sensorId\",\"temperature\":\"$sensorValue\"}" \
                http://$SERVER_IP:$SERVER_PORT
   
   sleep 1
done
