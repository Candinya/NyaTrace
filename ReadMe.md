<div align="center">

<img src="https://nyatrace.app/images/NyaTrace.svg" width="120" height="120" alt="NyaTrace Logo"/>

</div>

# NyaTrace

一个~~聊胜于无的~~可视化路由追踪工具（ Windows 平台）

## 运行截图

> 出于安全因素考虑，抹掉了敏感的 IP 。

![运行截图](https://file.nya.one/misskey/2d144605-150c-4039-98c1-7af713690cd0.png)

## 声明

1. 我们预先构建的可执行程序并不能保证在您的设备上也能功能完整地正常执行，如果您有任何问题，您可以考虑尝试自行构建，或是提出一个 issue 。
2. 程序使用的开发语言为 C++ ，开发环境为 Qt 5.15.2 ，构建工具使用 MSVC 2019 64bit ，具体构建流程可以参见项目的 CI 配置。
3. 目前程序支持的运行环境为 Windows 平台，其他跨平台的兼容性需求需要等待进一步的测试研发。
4. 目前支持的路由追踪模式为 IPv4 ，与 IPv6 相关的需求需要等待进一步的测试研发。
5. 严格禁止用于任何意义上与商业沾边的用途，包括但不仅限于二次封装与分发、商业化改造等。如果您有好的想法，欢迎随时开启一个 PR 。如果您有商业需要请自己从底层开发起，该封装的操作该处理的函数一看就明白了。一经发现有商业相关的情况项目立刻转为闭源。
6. 项目**唯一**的官网是 [nyatrace.app](https://nyatrace.app) ，其他都是假的，我们不对从假网站上的链接下载得到的任何一个二进制位的安全性负责。

## 使用

1. 从 Release 页面下载预先构建的可执行文件压缩包。
2. 从 MaxMind 下载 GeoIP2-City.mmdb 和 GeoIP2-ISP.mmdb 文件，放置在可执行文件目录的 mmdb 目录下。
3. 运行程序，输入需要路由追踪的地址，开始追踪。

## 提示

1. 如果您没有购买 GeoIP2 数据库，您也可以使用 GeoLite2 （即免费版的）数据库作为替代（ GeoIP2-ISP 请用 GeoLite2-ASN 代替），将其重命名为上文指定的格式即可，需要注意相关的结果和数据可能并不如完整版丰富准确。

## 后续的开发方向

1. ~~解决防火墙拦截包的问题~~ 已经解决
2. ~~多倍+并行发包以提升质量与追踪速度~~ 已经完成
3. ~~支持 IPv6 追踪~~ 已经完成
4. 集成一个地图服务（自建 OSM 部署）
5. 跨平台兼容

## 非常感谢

### 开源依赖

- [libmaxminddb](https://github.com/maxmind/libmaxminddb)
- [Qt for Open Source Development](https://www.qt.io/download-open-source)

### 应用图标

改自 [Nucleo](https://nucleoapp.com/) - `world-marker`
