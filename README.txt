1. ͬ�� OpenHarmony ����

https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/quickstart-standard-package-environment.md

���ղ���ͬ������ L2 ���롣


2. ���б���

./build/prebuilts_download.sh
./build.sh --product-name Hi3516DV300 --gn-args support_selinux=true


3. ������֤

1) ls -lZ /         # ����ǩ�Ƿ�ɹ�
2) ps -eZ           # ����ǩ�Ƿ�ɹ�
3) setenforce 1     # ���и��ֲ������۲��Ƿ����أ��Լ������Ƿ��� avc denied

demoloop ������ʱ����֤��

