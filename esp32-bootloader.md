# bootloader

```
esptool.py -p /dev/cu.usbserial-AI02KQCG erase_flash
esptool.py -p /dev/cu.usbserial-AI02KQCG write_flash --flash_freq 40m --flash_mode dio --flash_size 4MB 0x0 bootloader/bootloader.bin

esptool.py -p /dev/cu.usbserial-AI02KQCG write_flash --after hard_reset -z download.config

```


```
screen /dev/cu.usbserial-AI02KQCG 115200

```