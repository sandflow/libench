#!/bin/bash

set -e
set -x

DIR="$(cd "$(dirname "$0")" && pwd)"
LAUNCH_TEMPLATE_NAME=libench
PRIVATE_KEY_PATH=build/libench-worker.pem

INSTANCE_ID=$(aws ec2 run-instances \
  --launch-template LaunchTemplateName=${LAUNCH_TEMPLATE_NAME} \
   --output text \
   --query 'Instances[0].InstanceId')

aws ec2 wait instance-status-ok --instance-ids $INSTANCE_ID

PUBLIC_IP=$(aws ec2 describe-instances --instance-ids $INSTANCE_ID --output text --query 'Reservations[0].Instances[0].PublicIpAddress')

ssh -o StrictHostKeyChecking=no -i $PRIVATE_KEY_PATH -l ubuntu $PUBLIC_IP < $DIR/build.sh

aws ec2 terminate-instances --instance-ids $INSTANCE_ID

aws ec2 wait instance-terminated --instance-ids $INSTANCE_ID

