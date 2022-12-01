# NyaTrace

一个聊胜于无的可视化路由追踪工具（ Windows 平台）

## 提示

如果您发现除了最后一包之外其余的全都超时，您可能需要设置以下的防火墙规则，以放行 ICMP Time-to-live Exceeded 包和 ICMP Destination (Port) Unreachable 包，请以管理员权限执行以下代码：

```
netsh advfirewall firewall add rule name="All ICMP v4" dir=in action=allow protocol=icmpv4:any,any
netsh advfirewall firewall add rule name="All ICMP v6" dir=in action=allow protocol=icmpv6:any,any
```
