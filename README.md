# security_selinux

## Ŀ��

SELinux ����ȫ��ǿʽ Linux �� Security-Enhanced Linux ���� Linux ��ʷ����ܳ����°�ȫ��ϵͳ�� SELinux SIG �Ĺ���Ŀ���ǽ� SELinux ���� OpenHarmony ������������ OpenHarmony �� L2 �豸��ʵ�ַ��ʿ��ơ�

> 1. SELinux ��һ���ں��޸ĺ��û��ռ乤�ߣ����ṩ�˷��ʿ��ư�ȫ���Ի��ƣ�������ǿ�Ʒ��ʿ��ƣ� Mandatory Access Control �� MAC ����
> 2. SELinux �Ѿ�����ӵ����� Linux ���а��С�������ܹ���ͼ����ȫ���ߵ�ִ���밲ȫ���Է��룬�����漰ִ�а�ȫ���Ե������������

## �ֿ�

Ŀǰ�漰���Ĳֿ������¼�����

| �ֿ� | Դ��Ŀ¼ | ˵�� |
| --- | --- | --- |
| [security_selinux](https://gitee.com/shell_way/security_selinux.git) | `base/security/selinux/` | ���Ժ�һЩ���нӿ� |
| [third_party_selinux](https://gitee.com/openharmony-sig/third_party_selinux.git) | `third_party/selinux/` | SELinux �����ֿ� |
| [productdefine_common](https://gitee.com/hu-huifeng/productdefine_common.git) | `productdefine/common/` | ��� SELinux ������� |
| [third_party_toybox](https://gitee.com/hu-huifeng/third_party_toybox.git) | `third_party/toybox/` | ������ `ls` �� SELinux ֧�� |
| [startup_init_lite](https://gitee.com/shell_way/startup_init_lite.git) | `base/startup/init_lite/` | ϵͳ�������ز��Բ��ֻ�����ı�ǩ |
| [third_party_FreeBSD](https://gitee.com/shell_way/third_party_FreeBSD.git) | `third_party/FreeBSD/` | �ṩ fts �� |
| [third_party_pcre2](https://gitee.com/shell_way/third_party_pcre2.git) | `third_party/pcre2/` | �ṩ pcre2 �� |
| [build](https://gitee.com/openharmony/build.git) | `build/` | ������� |

> ��ǰδ�������ߣ�����������һЩ���˲ֿ⡣

## �ܹ�

![����ܹ�](docs/images/����ܹ�.png)

�� [third_party_selinux](https://gitee.com/openharmony-sig/third_party_selinux.git) ��ʹ���������ĸ� SELinux ��������������������������

| ��� | ��Դ | ���� | ��ʽ |
| --- | --- | --- | --- |
| `checkpolicy/` | [selinux/checkpolicy](https://github.com/SELinuxProject/selinux/tree/cf853c1a0c2328ad6c62fb2b2cc55d4926301d6b/checkpolicy) | `checkpolicy` | ��ִ���ļ� |
| `libselinux/` | [selinux/libselinux](https://github.com/SELinuxProject/selinux/tree/cf853c1a0c2328ad6c62fb2b2cc55d4926301d6b/libselinux) | `libselinux.so`��`getenforce`��`setenforce` | ��̬�� |
| `libsepol/` | [selinux/libsepol](https://github.com/SELinuxProject/selinux/tree/cf853c1a0c2328ad6c62fb2b2cc55d4926301d6b/libsepol) | �ṩ�ڲ�ʹ�õ� API | ��̬�� |
| `seclic/` | [selinux/seclic](https://github.com/SELinuxProject/selinux/tree/cf853c1a0c2328ad6c62fb2b2cc55d4926301d6b/secilc) | `seclic` | ��ִ���ļ� |
| `depends/fts/` | [openbsd](https://github.com/openbsd/src/tree/e8835b178a3e9df00c1c1fe0b9875fc5ef5a7854) | �ṩ fts | ��̬���� |
| `depends/pcre/pcre2/` | [pcre2](https://github.com/PhilipHazel/pcre2/tree/2ae7c30b95d63ecbaff6727eaff7c3a6a3969d56) | �ṩ `libpcre2.so` | ��̬�� |

## ����

### ͬ�� OpenHarmony ����

����[���ĵ�](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/quickstart-standard-package-environment.md)ͬ������ L2 ���롣

### ͬ������ֿ�

| Ŀ¼ | �ֿ� |
| --- | --- |
| `base/security/selinux/` | `https://gitee.com/shell_way/security_selinux.git` |
| `third_party/selinux/` |  `https://gitee.com/openharmony-sig/third_party_selinux.git` |
| `productdefine/common/` | `https://gitee.com/hu-huifeng/productdefine_common.git` |
| `third_party/toybox/` | `https://gitee.com/hu-huifeng/third_party_toybox.git` |
| `base/startup/init_lite/` | `https://gitee.com/shell_way/startup_init_lite.git` |
| `third_party/FreeBSD/` | `https://gitee.com/shell_way/third_party_FreeBSD.git` |
| `third_party_pcre2` | `https://gitee.com/shell_way/third_party_pcre2.git` |
| `build/` | `https://gitee.com/shell_way/build.git` |

### ���б���

```
./build/prebuilts_download.sh
./build.sh --product-name Hi3516DV300 --gn-args "support_selinux=true"
```

### ������֤

��������¼�� Hi3516DV300 �������ϡ�

```
ls -lZ /         # �鿴�ļ���ǩ�Ƿ�ɹ�
ps -eZ           # �鿴���̱�ǩ�Ƿ�ɹ�
setenforce 1     # ���и��ֲ������۲��Ƿ����أ��Լ������Ƿ��� avc denied
```

