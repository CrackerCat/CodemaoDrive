# CodemaoDrive

#### 你猫云，支持任意文件的全速上传与下载

![preview.png](https://i.loli.net/2020/02/26/i1sknNGF8rJQS2v.png)

## 特色

-   轻量：无复杂依赖，资源占用少
-   自由：无文件格式与大小限制，无容量限制
-   快速：支持多线程传输与断点续传，同时借助你猫的 CDN 资源，能最大化地利用网络环境进行上传与下载

## 如何使用

### 编译

首先 clone 项目

```shell
git clone https://github.com/zHElEARN/CodemaoDrive.git
or
git clone git@github.com:zHElEARN/CodemaoDrive.git

git submodule update --init --recursive
```

然后使用 cmake 生成编译文件，并使用`Visual Studio 2019`进行编译

```shell
cd CodemaoDrive
mkdir build
cd build
cmake -A "Win32" ..
devenv.exe CodemaoDrive.sln
```

### Usage

```shell
▶ .\CodemaoDrive.exe  --help
Usage: CodemaoMaterial [options] method

Positional arguments:
method          选择上传/下载方法[Required]

Optional arguments:
-h --help       show this help message and exit
-d --data       要上传/下载 的 文件名/文件键值[Required]
```

### 上传

```shell
CodemaoDrive.exe upload -d filename
```

上传完毕后，终端会打印一串键值 于下载或分享，请妥善保管

### 下载

```shell
CodemaoDrive.exe download -d key
```

通过键值进行下载

### 历史记录

```shell
CodemaoDrive.exe histroy
```

获得保存的历史记录

## 免责声明

请自行对重要文件做好本地备份

请勿使用本项目上传不符合社会主义核心价值观的文件

请合理使用本项目，避免对编程猫的存储与带宽资源造成无意义的浪费

该项目仅用于学习和技术交流，开发者不承担任何由使用者的行为带来的法律责任

## License

GNU Lesser General Public License v3.0
