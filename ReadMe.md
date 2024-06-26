<div align="center">

<img src="./installer/config/NyaTrace.png" width="120" height="120" alt="NyaTrace Logo"/>

</div>

# NyaTrace

可视化路由追踪工具

## 运行截图

![运行截图](https://candymade.net/assets/screenshots/nyatrace/main.png)

## 声明

构建的可执行程序会不定期在不同环境下测试，但并不能保证在您的设备上也能功能完整地正常执行，如果您有任何问题，您可以提出一个 issue 。

## 使用

1. 从 Release 页面下载预先构建的可执行文件压缩包。
2. （可选）如果您需要使用详细信息查询和地理位置定位功能，请从 MaxMind 下载 GeoIP2-City.mmdb 和 GeoIP2-ISP.mmdb 文件，放置在可执行文件目录的 mmdb 目录下。没有它们也能正常执行追踪功能。
3. 运行程序，输入需要路由追踪的地址，开始追踪。

## 提示

1. 如果您没有 GeoIP2 数据库，您也可以使用 GeoLite2 （即免费版的）数据库作为替代（ GeoIP2-ISP 请用 GeoLite2-ASN 代替），将其重命名为上文指定的格式即可。
2. 如果觉得现在这个主题不好看，您可以参照 [qt-material#25](https://github.com/UN-GCPDS/qt-material/issues/25#issuecomment-835368713) 中给出的主题生成步骤配置您喜欢的主题，并替换掉 theme 目录下的 qss 样式文件（请记得删去生成文件中的字体设置，要不然中文回落到宋体会很难看）。如果您喜欢 Qt 默认的主题，可以直接删除整个 theme 目录。
3. 配置文件会被保存在 `FOLDERID_RoamingAppData\Nya Candy\NyaTrace.ini` 这个文件中，例如对我来说是 `C:\Users\Candinya\AppData\Roaming\Nya Candy\NyaTrace.ini` 这里。

## 后续的开发方向

- [x] ~~解决防火墙拦截包的问题~~
- [x] ~~多倍+并行发包以提升质量与追踪速度~~
- [x] ~~支持 IPv6 追踪~~
- [x] ~~集成一个地图服务~~
- [x] ~~读写配置项~~
- [x] ~~主题与美化~~
- [ ] 跨平台兼容
- [ ] 支持其他追踪模式 (TCP & UDP)

## 非常感谢

### 开源依赖

- [libmaxminddb](https://github.com/maxmind/libmaxminddb)
- [qt-material](https://github.com/UN-GCPDS/qt-material)
- [Qt for Open Source Development](https://www.qt.io/download-open-source)

### 应用图标

改自 [Nucleo](https://nucleoapp.com/) - `world-marker`

### 代码参考

- [WinMTR (Redux)](https://github.com/White-Tiger/WinMTR)
