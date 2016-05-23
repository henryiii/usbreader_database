
DEVICE=/dev/ttyACM0
MESSAGE="1"

echo -e ${MESSAGE} | sudo tee ${DEVICE}


