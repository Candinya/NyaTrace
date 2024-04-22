<div align="center">

<img src="./installer/config/NyaTrace.png" width="120" height="120" alt="NyaTrace Logo"/>

</div>

# NyaTrace

一个可视化路由追踪工具（ Windows 平台）

## 运行截图

![运行截图](https://candymade.net/assets/screenshots/nyatrace/main.png)

## 声明

1. 我们预先构建的可执行程序并不能保证在您的设备上也能功能完整地正常执行，如果您有任何问题，您可以提出一个 issue 。
2. 目前程序仅支持 Windows 平台。
3. 严禁商用。

## 使用

1. 从 Release 页面下载预先构建的可执行文件压缩包。
2. 从 MaxMind 下载 GeoIP2-City.mmdb 和 GeoIP2-ISP.mmdb 文件，放置在可执行文件目录的 mmdb 目录下。
3. 运行程序，输入需要路由追踪的地址，开始追踪。

## 提示

1. 如果您没有 GeoIP2 数据库，您也可以使用 GeoLite2 （即免费版的）数据库作为替代（ GeoIP2-ISP 请用 GeoLite2-ASN 代替），将其重命名为上文指定的格式即可。
2. 如果觉得现在这个主题不好看，您可以参照 [qt-material#25](https://github.com/UN-GCPDS/qt-material/issues/25#issuecomment-835368713) 中给出的主题生成步骤配置您喜欢的主题，并替换掉 theme 目录下的 qss 样式文件（请记得删去生成文件中的字体设置，要不然中文回落到宋体会很难看）。如果您喜欢 Qt 默认的主题，可以直接删除整个 theme 目录。

## 后续的开发方向

1. ~~解决防火墙拦截包的问题~~ 已经解决
2. ~~多倍+并行发包以提升质量与追踪速度~~ 已经完成
3. ~~支持 IPv6 追踪~~ 已经完成
4. ~~集成一个地图服务~~ 已经完成
5. 读写配置项
6. ~~主题与美化~~ 已经完成
7. 跨平台兼容

## 非常感谢

### 开源依赖

- [libmaxminddb](https://github.com/maxmind/libmaxminddb)
- [qt-material](https://github.com/UN-GCPDS/qt-material)
- [Qt for Open Source Development](https://www.qt.io/download-open-source)

### 应用图标

改自 [Nucleo](https://nucleoapp.com/) - `world-marker`

### 代码参考

- [WinMTR (Redux)](https://github.com/White-Tiger/WinMTR)
