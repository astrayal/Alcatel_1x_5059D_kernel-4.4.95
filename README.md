###########################################################################################

Now translated to English from Russian thanks google !

###########################################################################################

Note before using this update path vars in build.sh to match your env.

Before the assembly, we do export export TARGET_BUILD_VARIANT = user

And build the kernel: build.sh

If the build was successful, go to the my_kernel folder, the folder should contain the working boot.img and run: repack.sh pick up the new boot folder and modules from the modules folder

bt_drv.ko

fmradio_drv.ko

gps_drv.ko

wlan_drv.ko

wmt_chrdev_wifi.ko

wmt_drv.ko

Thanks "vgdn1942" for the instruction

https://4pda.ru/forum/index.php?showtopic=583114&view=findpost&p=86045582

#######################################################################
# Alcatel_1x_5059D_kernel-4.4.95
-------------------------------------------------------------------------

Перед сборкой делаем экспорт
export TARGET_BUILD_VARIANT=user

И собираем ядро: build.sh

Если сборка прошла успешно, переходим в папку my_kernel, в папке должен лежать рабочий boot.img
и запускаем: repack.sh
забираем в папке новый boot и модули из папки modules

bt_drv.ko

fmradio_drv.ko

gps_drv.ko

wlan_drv.ko

wmt_chrdev_wifi.ko

wmt_drv.ko

-------------------------------------------------------------------------
Спасибо "vgdn1942" за инструкцию

https://4pda.ru/forum/index.php?showtopic=583114&view=findpost&p=86045582

