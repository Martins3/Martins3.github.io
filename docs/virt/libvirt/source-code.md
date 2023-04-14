# 简要分析下 source code 的位置

```txt
🧀  tokei -f
======================================================================================================================================================
 Language                                                                                   Files        Lines         Code     Comments       Blanks
======================================================================================================================================================
 C                                                                                             42       128900        93593        11113        24194
------------------------------------------------------------------------------------------------------------------------------------------------------
 ./qemu_hostdev.c                                                                                          461          348           26           87
 ./qemu_vhost_user.c                                                                                       454          332           22          100
 ./qemu_slirp.c                                                                                            354          257           20           77
 ./qemu_passt.c                                                                                            292          197           31           64
 ./qemu_domainjob.c                                                                                        780          615           27          138
 ./qemu_interface.c                                                                                        776          495          177          104
 ./qemu_alias.c                                                                                            879          622          113          144
 ./qemu_security.c                                                                                         673          489           40          144
 ./qemu_vhost_user_gpu.c                                                                                   251          144           62           45
 ./qemu_virtiofs.c                                                                                         369          273           18           78
 ./qemu_migration_cookie.c                                                                                1662         1266           61          335
 ./qemu_cgroup.c                                                                                           933          674           63          196
 ./qemu_interop_config.c                                                                                   162           96           29           37
 ./qemu_monitor_text.c                                                                                      88           54           20           14
 ./qemu_saveimage.c                                                                                        759          517          117          125
 ./qemu_dbus.c                                                                                             302          221           17           64
 ./qemu_qapi.c                                                                                             563          341          101          121
 ./qemu_shim.c                                                                                             418          322           34           62
 ./qemu_extdevice.c                                                                                        378          248           47           83
 ./qemu_fd.c                                                                                               403          202          126           75
 ./qemu_migration_params.c                                                                                1542         1105          128          309
 ./qemu_tpm.c                                                                                             1160          701          265          194
 ./qemu_checkpoint.c                                                                                       921          673           63          185
 ./qemu_firmware.c                                                                                        1929         1401          173          355
 ./qemu_namespace.c                                                                                       1759         1252          129          378
 ./qemu_blockjob.c                                                                                        1730         1097          281          352
 ./qemu_block.c                                                                                           3617         2355          593          669
 ./qemu_conf.c                                                                                            1656         1240          103          313
 ./qemu_monitor.c                                                                                         4520         2997          513         1010
 ./qemu_domain_address.c                                                                                  3376         2185          674          517
 ./qemu_backup.c                                                                                          1186          863           80          243
 ./qemu_agent.c                                                                                           2572         1800          318          454
 ./qemu_snapshot.c                                                                                        3212         2354          278          580
 ./qemu_migration.c                                                                                       7170         5398          609         1163
 ./qemu_hotplug.c                                                                                         6723         4912          540         1271
 ./qemu_validate.c                                                                                        5247         4287          230          730
 ./qemu_process.c                                                                                         9543         6760          955         1828
 ./qemu_capabilities.c                                                                                    6830         5023          528         1279
 ./qemu_driver.c                                                                                         20843        16000          787         4056
 ./qemu_command.c                                                                                        10927         8158          733         2036
 ./qemu_monitor_json.c                                                                                    8927         6625          491         1811
 ./qemu_domain.c                                                                                         12553         8694         1491         2368
```
