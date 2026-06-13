
test.out:     file format elf64-x86-64


Disassembly of section .text:

0000000000401000 <a>:
  401000:	55                   	push   %rbp
  401001:	48 89 e5             	mov    %rsp,%rbp
  401004:	48 83 ec 10          	sub    $0x10,%rsp
  401008:	bf 04 00 00 00       	mov    $0x4,%edi
  40100d:	e8 12 04 00 00       	callq  401424 <crt_malloc>
  401012:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401016:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40101a:	c6 00 43             	movb   $0x43,(%rax)
  40101d:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401021:	48 83 c0 01          	add    $0x1,%rax
  401025:	c6 00 52             	movb   $0x52,(%rax)
  401028:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40102c:	48 83 c0 02          	add    $0x2,%rax
  401030:	c6 00 54             	movb   $0x54,(%rax)
  401033:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401037:	48 83 c0 03          	add    $0x3,%rax
  40103b:	c6 00 00             	movb   $0x0,(%rax)
  40103e:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401042:	48 89 c6             	mov    %rax,%rsi
  401045:	48 8d 3d b4 0f 00 00 	lea    0xfb4(%rip),%rdi        # 402000 <etext+0x346>
  40104c:	b8 00 00 00 00       	mov    $0x0,%eax
  401051:	e8 85 07 00 00       	callq  4017db <printf>
  401056:	90                   	nop
  401057:	c9                   	leaveq 
  401058:	c3                   	retq   

0000000000401059 <b>:
  401059:	55                   	push   %rbp
  40105a:	48 89 e5             	mov    %rsp,%rbp
  40105d:	48 83 ec 10          	sub    $0x10,%rsp
  401061:	48 b8 73 61 66 73 64 	movabs $0x6461666473666173,%rax
  401068:	66 61 64 
  40106b:	48 89 45 f7          	mov    %rax,-0x9(%rbp)
  40106f:	c6 45 ff 00          	movb   $0x0,-0x1(%rbp)
  401073:	48 8d 45 f7          	lea    -0x9(%rbp),%rax
  401077:	be 01 00 00 00       	mov    $0x1,%esi
  40107c:	48 89 c7             	mov    %rax,%rdi
  40107f:	e8 39 06 00 00       	callq  4016bd <puts>
  401084:	90                   	nop
  401085:	c9                   	leaveq 
  401086:	c3                   	retq   

0000000000401087 <c>:
  401087:	55                   	push   %rbp
  401088:	48 89 e5             	mov    %rsp,%rbp
  40108b:	48 83 ec 30          	sub    $0x30,%rsp
  40108f:	b8 00 00 00 00       	mov    $0x0,%eax
  401094:	e8 f1 02 00 00       	callq  40138a <crt_print_memory_usage>
  401099:	bf e8 03 00 00       	mov    $0x3e8,%edi
  40109e:	e8 81 03 00 00       	callq  401424 <crt_malloc>
  4010a3:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  4010a7:	b8 00 00 00 00       	mov    $0x0,%eax
  4010ac:	e8 d9 02 00 00       	callq  40138a <crt_print_memory_usage>
  4010b1:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4010b5:	48 89 c7             	mov    %rax,%rdi
  4010b8:	e8 c7 01 00 00       	callq  401284 <crt_free>
  4010bd:	b8 00 00 00 00       	mov    $0x0,%eax
  4010c2:	e8 c3 02 00 00       	callq  40138a <crt_print_memory_usage>
  4010c7:	bf e8 03 00 00       	mov    $0x3e8,%edi
  4010cc:	e8 53 03 00 00       	callq  401424 <crt_malloc>
  4010d1:	48 89 45 f0          	mov    %rax,-0x10(%rbp)
  4010d5:	b8 00 00 00 00       	mov    $0x0,%eax
  4010da:	e8 ab 02 00 00       	callq  40138a <crt_print_memory_usage>
  4010df:	bf e8 03 00 00       	mov    $0x3e8,%edi
  4010e4:	e8 3b 03 00 00       	callq  401424 <crt_malloc>
  4010e9:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
  4010ed:	b8 00 00 00 00       	mov    $0x0,%eax
  4010f2:	e8 93 02 00 00       	callq  40138a <crt_print_memory_usage>
  4010f7:	bf e8 03 00 00       	mov    $0x3e8,%edi
  4010fc:	e8 23 03 00 00       	callq  401424 <crt_malloc>
  401101:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
  401105:	b8 00 00 00 00       	mov    $0x0,%eax
  40110a:	e8 7b 02 00 00       	callq  40138a <crt_print_memory_usage>
  40110f:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401113:	48 89 c7             	mov    %rax,%rdi
  401116:	e8 69 01 00 00       	callq  401284 <crt_free>
  40111b:	b8 00 00 00 00       	mov    $0x0,%eax
  401120:	e8 65 02 00 00       	callq  40138a <crt_print_memory_usage>
  401125:	bf d0 07 00 00       	mov    $0x7d0,%edi
  40112a:	e8 f5 02 00 00       	callq  401424 <crt_malloc>
  40112f:	48 89 45 d8          	mov    %rax,-0x28(%rbp)
  401133:	b8 00 00 00 00       	mov    $0x0,%eax
  401138:	e8 4d 02 00 00       	callq  40138a <crt_print_memory_usage>
  40113d:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  401141:	48 89 c7             	mov    %rax,%rdi
  401144:	e8 3b 01 00 00       	callq  401284 <crt_free>
  401149:	b8 00 00 00 00       	mov    $0x0,%eax
  40114e:	e8 37 02 00 00       	callq  40138a <crt_print_memory_usage>
  401153:	90                   	nop
  401154:	c9                   	leaveq 
  401155:	c3                   	retq   

0000000000401156 <d>:
  401156:	55                   	push   %rbp
  401157:	48 89 e5             	mov    %rsp,%rbp
  40115a:	48 8d 35 17 2f 00 00 	lea    0x2f17(%rip),%rsi        # 404078 <_end>
  401161:	48 8d 3d b0 0e 00 00 	lea    0xeb0(%rip),%rdi        # 402018 <etext+0x35e>
  401168:	b8 00 00 00 00       	mov    $0x0,%eax
  40116d:	e8 69 06 00 00       	callq  4017db <printf>
  401172:	48 8d 35 41 0b 00 00 	lea    0xb41(%rip),%rsi        # 401cba <etext>
  401179:	48 8d 3d a6 0e 00 00 	lea    0xea6(%rip),%rdi        # 402026 <etext+0x36c>
  401180:	b8 00 00 00 00       	mov    $0x0,%eax
  401185:	e8 51 06 00 00       	callq  4017db <printf>
  40118a:	48 8d 35 81 2e 00 00 	lea    0x2e81(%rip),%rsi        # 404012 <_edata>
  401191:	48 8d 3d 9d 0e 00 00 	lea    0xe9d(%rip),%rdi        # 402035 <etext+0x37b>
  401198:	b8 00 00 00 00       	mov    $0x0,%eax
  40119d:	e8 39 06 00 00       	callq  4017db <printf>
  4011a2:	b8 00 00 00 00       	mov    $0x0,%eax
  4011a7:	e8 de 01 00 00       	callq  40138a <crt_print_memory_usage>
  4011ac:	90                   	nop
  4011ad:	5d                   	pop    %rbp
  4011ae:	c3                   	retq   

00000000004011af <e>:
  4011af:	55                   	push   %rbp
  4011b0:	48 89 e5             	mov    %rsp,%rbp
  4011b3:	eb fe                	jmp    4011b3 <e+0x4>

00000000004011b5 <g>:
  4011b5:	55                   	push   %rbp
  4011b6:	48 89 e5             	mov    %rsp,%rbp
  4011b9:	48 83 ec 20          	sub    $0x20,%rsp
  4011bd:	89 7d ec             	mov    %edi,-0x14(%rbp)
  4011c0:	48 89 75 e0          	mov    %rsi,-0x20(%rbp)
  4011c4:	8b 45 ec             	mov    -0x14(%rbp),%eax
  4011c7:	89 c6                	mov    %eax,%esi
  4011c9:	48 8d 3d 74 0e 00 00 	lea    0xe74(%rip),%rdi        # 402044 <etext+0x38a>
  4011d0:	b8 00 00 00 00       	mov    $0x0,%eax
  4011d5:	e8 01 06 00 00       	callq  4017db <printf>
  4011da:	bf e8 03 00 00       	mov    $0x3e8,%edi
  4011df:	e8 40 02 00 00       	callq  401424 <crt_malloc>
  4011e4:	48 89 45 f0          	mov    %rax,-0x10(%rbp)
  4011e8:	48 8d 35 59 0e 00 00 	lea    0xe59(%rip),%rsi        # 402048 <etext+0x38e>
  4011ef:	48 8d 3d 5d 0e 00 00 	lea    0xe5d(%rip),%rdi        # 402053 <etext+0x399>
  4011f6:	b8 00 00 00 00       	mov    $0x0,%eax
  4011fb:	e8 db 05 00 00       	callq  4017db <printf>
  401200:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%rbp)
  401207:	eb 49                	jmp    401252 <g+0x9d>
  401209:	8b 45 fc             	mov    -0x4(%rbp),%eax
  40120c:	48 98                	cltq   
  40120e:	48 8d 14 c5 00 00 00 	lea    0x0(,%rax,8),%rdx
  401215:	00 
  401216:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  40121a:	48 01 d0             	add    %rdx,%rax
  40121d:	48 8b 10             	mov    (%rax),%rdx
  401220:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  401224:	48 89 d6             	mov    %rdx,%rsi
  401227:	48 89 c7             	mov    %rax,%rdi
  40122a:	e8 b4 09 00 00       	callq  401be3 <strcpy>
  40122f:	48 85 c0             	test   %rax,%rax
  401232:	74 1a                	je     40124e <g+0x99>
  401234:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  401238:	8b 45 fc             	mov    -0x4(%rbp),%eax
  40123b:	89 c6                	mov    %eax,%esi
  40123d:	48 8d 3d 13 0e 00 00 	lea    0xe13(%rip),%rdi        # 402057 <etext+0x39d>
  401244:	b8 00 00 00 00       	mov    $0x0,%eax
  401249:	e8 8d 05 00 00       	callq  4017db <printf>
  40124e:	83 45 fc 01          	addl   $0x1,-0x4(%rbp)
  401252:	8b 45 fc             	mov    -0x4(%rbp),%eax
  401255:	3b 45 ec             	cmp    -0x14(%rbp),%eax
  401258:	7c af                	jl     401209 <g+0x54>
  40125a:	90                   	nop
  40125b:	c9                   	leaveq 
  40125c:	c3                   	retq   

000000000040125d <main>:
  40125d:	55                   	push   %rbp
  40125e:	48 89 e5             	mov    %rsp,%rbp
  401261:	48 83 ec 10          	sub    $0x10,%rsp
  401265:	89 7d fc             	mov    %edi,-0x4(%rbp)
  401268:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  40126c:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  401270:	8b 45 fc             	mov    -0x4(%rbp),%eax
  401273:	48 89 d6             	mov    %rdx,%rsi
  401276:	89 c7                	mov    %eax,%edi
  401278:	e8 38 ff ff ff       	callq  4011b5 <g>
  40127d:	b8 00 00 00 00       	mov    $0x0,%eax
  401282:	c9                   	leaveq 
  401283:	c3                   	retq   

0000000000401284 <crt_free>:
  401284:	55                   	push   %rbp
  401285:	48 89 e5             	mov    %rsp,%rbp
  401288:	48 83 ec 20          	sub    $0x20,%rsp
  40128c:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  401290:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401294:	48 83 e8 18          	sub    $0x18,%rax
  401298:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  40129c:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4012a0:	8b 00                	mov    (%rax),%eax
  4012a2:	3d ab ab ab ab       	cmp    $0xabababab,%eax
  4012a7:	75 16                	jne    4012bf <crt_free+0x3b>
  4012a9:	48 8d 3d b0 0d 00 00 	lea    0xdb0(%rip),%rdi        # 402060 <etext+0x3a6>
  4012b0:	b8 00 00 00 00       	mov    $0x0,%eax
  4012b5:	e8 21 05 00 00       	callq  4017db <printf>
  4012ba:	e9 c9 00 00 00       	jmpq   401388 <crt_free+0x104>
  4012bf:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4012c3:	c7 00 ab ab ab ab    	movl   $0xabababab,(%rax)
  4012c9:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4012cd:	48 8b 40 08          	mov    0x8(%rax),%rax
  4012d1:	48 85 c0             	test   %rax,%rax
  4012d4:	74 40                	je     401316 <crt_free+0x92>
  4012d6:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4012da:	48 8b 40 08          	mov    0x8(%rax),%rax
  4012de:	8b 00                	mov    (%rax),%eax
  4012e0:	3d ab ab ab ab       	cmp    $0xabababab,%eax
  4012e5:	75 2f                	jne    401316 <crt_free+0x92>
  4012e7:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4012eb:	8b 50 04             	mov    0x4(%rax),%edx
  4012ee:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4012f2:	48 8b 40 08          	mov    0x8(%rax),%rax
  4012f6:	8b 40 04             	mov    0x4(%rax),%eax
  4012f9:	01 c2                	add    %eax,%edx
  4012fb:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4012ff:	89 50 04             	mov    %edx,0x4(%rax)
  401302:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401306:	48 8b 40 08          	mov    0x8(%rax),%rax
  40130a:	48 8b 50 08          	mov    0x8(%rax),%rdx
  40130e:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401312:	48 89 50 08          	mov    %rdx,0x8(%rax)
  401316:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40131a:	48 8b 40 10          	mov    0x10(%rax),%rax
  40131e:	48 85 c0             	test   %rax,%rax
  401321:	74 65                	je     401388 <crt_free+0x104>
  401323:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401327:	48 8b 40 10          	mov    0x10(%rax),%rax
  40132b:	8b 00                	mov    (%rax),%eax
  40132d:	3d ab ab ab ab       	cmp    $0xabababab,%eax
  401332:	75 54                	jne    401388 <crt_free+0x104>
  401334:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401338:	48 8b 40 10          	mov    0x10(%rax),%rax
  40133c:	48 8b 55 f8          	mov    -0x8(%rbp),%rdx
  401340:	48 8b 52 08          	mov    0x8(%rdx),%rdx
  401344:	48 89 50 08          	mov    %rdx,0x8(%rax)
  401348:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40134c:	48 8b 40 08          	mov    0x8(%rax),%rax
  401350:	48 85 c0             	test   %rax,%rax
  401353:	74 14                	je     401369 <crt_free+0xe5>
  401355:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401359:	48 8b 40 08          	mov    0x8(%rax),%rax
  40135d:	48 8b 55 f8          	mov    -0x8(%rbp),%rdx
  401361:	48 8b 52 10          	mov    0x10(%rdx),%rdx
  401365:	48 89 50 10          	mov    %rdx,0x10(%rax)
  401369:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40136d:	48 8b 40 10          	mov    0x10(%rax),%rax
  401371:	8b 48 04             	mov    0x4(%rax),%ecx
  401374:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401378:	8b 50 04             	mov    0x4(%rax),%edx
  40137b:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40137f:	48 8b 40 10          	mov    0x10(%rax),%rax
  401383:	01 ca                	add    %ecx,%edx
  401385:	89 50 04             	mov    %edx,0x4(%rax)
  401388:	c9                   	leaveq 
  401389:	c3                   	retq   

000000000040138a <crt_print_memory_usage>:
  40138a:	55                   	push   %rbp
  40138b:	48 89 e5             	mov    %rsp,%rbp
  40138e:	48 83 ec 10          	sub    $0x10,%rsp
  401392:	48 8b 05 87 2c 00 00 	mov    0x2c87(%rip),%rax        # 404020 <list_head>
  401399:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  40139d:	48 8d 3d d5 0c 00 00 	lea    0xcd5(%rip),%rdi        # 402079 <etext+0x3bf>
  4013a4:	b8 00 00 00 00       	mov    $0x0,%eax
  4013a9:	e8 2d 04 00 00       	callq  4017db <printf>
  4013ae:	eb 59                	jmp    401409 <crt_print_memory_usage+0x7f>
  4013b0:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4013b4:	8b 00                	mov    (%rax),%eax
  4013b6:	3d cd cd cd cd       	cmp    $0xcdcdcdcd,%eax
  4013bb:	75 21                	jne    4013de <crt_print_memory_usage+0x54>
  4013bd:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4013c1:	8b 50 04             	mov    0x4(%rax),%edx
  4013c4:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4013c8:	48 89 c6             	mov    %rax,%rsi
  4013cb:	48 8d 3d b2 0c 00 00 	lea    0xcb2(%rip),%rdi        # 402084 <etext+0x3ca>
  4013d2:	b8 00 00 00 00       	mov    $0x0,%eax
  4013d7:	e8 ff 03 00 00       	callq  4017db <printf>
  4013dc:	eb 1f                	jmp    4013fd <crt_print_memory_usage+0x73>
  4013de:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4013e2:	8b 50 04             	mov    0x4(%rax),%edx
  4013e5:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4013e9:	48 89 c6             	mov    %rax,%rsi
  4013ec:	48 8d 3d 9d 0c 00 00 	lea    0xc9d(%rip),%rdi        # 402090 <etext+0x3d6>
  4013f3:	b8 00 00 00 00       	mov    $0x0,%eax
  4013f8:	e8 de 03 00 00       	callq  4017db <printf>
  4013fd:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401401:	48 8b 40 08          	mov    0x8(%rax),%rax
  401405:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401409:	48 83 7d f8 00       	cmpq   $0x0,-0x8(%rbp)
  40140e:	75 a0                	jne    4013b0 <crt_print_memory_usage+0x26>
  401410:	48 8d 3d 62 0c 00 00 	lea    0xc62(%rip),%rdi        # 402079 <etext+0x3bf>
  401417:	b8 00 00 00 00       	mov    $0x0,%eax
  40141c:	e8 ba 03 00 00       	callq  4017db <printf>
  401421:	90                   	nop
  401422:	c9                   	leaveq 
  401423:	c3                   	retq   

0000000000401424 <crt_malloc>:
  401424:	55                   	push   %rbp
  401425:	48 89 e5             	mov    %rsp,%rbp
  401428:	89 7d ec             	mov    %edi,-0x14(%rbp)
  40142b:	83 7d ec 00          	cmpl   $0x0,-0x14(%rbp)
  40142f:	75 0a                	jne    40143b <crt_malloc+0x17>
  401431:	b8 00 00 00 00       	mov    $0x0,%eax
  401436:	e9 19 01 00 00       	jmpq   401554 <crt_malloc+0x130>
  40143b:	48 8b 05 de 2b 00 00 	mov    0x2bde(%rip),%rax        # 404020 <list_head>
  401442:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401446:	e9 f9 00 00 00       	jmpq   401544 <crt_malloc+0x120>
  40144b:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40144f:	8b 00                	mov    (%rax),%eax
  401451:	3d cd cd cd cd       	cmp    $0xcdcdcdcd,%eax
  401456:	75 11                	jne    401469 <crt_malloc+0x45>
  401458:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40145c:	48 8b 40 08          	mov    0x8(%rax),%rax
  401460:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401464:	e9 db 00 00 00       	jmpq   401544 <crt_malloc+0x120>
  401469:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40146d:	8b 40 04             	mov    0x4(%rax),%eax
  401470:	89 c2                	mov    %eax,%edx
  401472:	8b 45 ec             	mov    -0x14(%rbp),%eax
  401475:	48 83 c0 18          	add    $0x18,%rax
  401479:	48 39 c2             	cmp    %rax,%rdx
  40147c:	76 2c                	jbe    4014aa <crt_malloc+0x86>
  40147e:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401482:	8b 40 04             	mov    0x4(%rax),%eax
  401485:	89 c2                	mov    %eax,%edx
  401487:	8b 45 ec             	mov    -0x14(%rbp),%eax
  40148a:	48 83 c0 30          	add    $0x30,%rax
  40148e:	48 39 c2             	cmp    %rax,%rdx
  401491:	77 17                	ja     4014aa <crt_malloc+0x86>
  401493:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401497:	c7 00 cd cd cd cd    	movl   $0xcdcdcdcd,(%rax)
  40149d:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4014a1:	48 83 c0 18          	add    $0x18,%rax
  4014a5:	e9 aa 00 00 00       	jmpq   401554 <crt_malloc+0x130>
  4014aa:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4014ae:	8b 40 04             	mov    0x4(%rax),%eax
  4014b1:	89 c2                	mov    %eax,%edx
  4014b3:	8b 45 ec             	mov    -0x14(%rbp),%eax
  4014b6:	48 83 c0 30          	add    $0x30,%rax
  4014ba:	48 39 c2             	cmp    %rax,%rdx
  4014bd:	76 79                	jbe    401538 <crt_malloc+0x114>
  4014bf:	8b 45 ec             	mov    -0x14(%rbp),%eax
  4014c2:	48 8d 50 18          	lea    0x18(%rax),%rdx
  4014c6:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4014ca:	48 01 d0             	add    %rdx,%rax
  4014cd:	48 89 45 f0          	mov    %rax,-0x10(%rbp)
  4014d1:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  4014d5:	48 8b 55 f8          	mov    -0x8(%rbp),%rdx
  4014d9:	48 89 50 10          	mov    %rdx,0x10(%rax)
  4014dd:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4014e1:	48 8b 50 08          	mov    0x8(%rax),%rdx
  4014e5:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  4014e9:	48 89 50 08          	mov    %rdx,0x8(%rax)
  4014ed:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  4014f1:	c7 00 ab ab ab ab    	movl   $0xabababab,(%rax)
  4014f7:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4014fb:	8b 40 04             	mov    0x4(%rax),%eax
  4014fe:	2b 45 ec             	sub    -0x14(%rbp),%eax
  401501:	8d 50 e8             	lea    -0x18(%rax),%edx
  401504:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  401508:	89 50 04             	mov    %edx,0x4(%rax)
  40150b:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40150f:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  401513:	48 89 50 08          	mov    %rdx,0x8(%rax)
  401517:	8b 45 ec             	mov    -0x14(%rbp),%eax
  40151a:	8d 50 18             	lea    0x18(%rax),%edx
  40151d:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401521:	89 50 04             	mov    %edx,0x4(%rax)
  401524:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401528:	c7 00 cd cd cd cd    	movl   $0xcdcdcdcd,(%rax)
  40152e:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401532:	48 83 c0 18          	add    $0x18,%rax
  401536:	eb 1c                	jmp    401554 <crt_malloc+0x130>
  401538:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40153c:	48 8b 40 08          	mov    0x8(%rax),%rax
  401540:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401544:	48 83 7d f8 00       	cmpq   $0x0,-0x8(%rbp)
  401549:	0f 85 fc fe ff ff    	jne    40144b <crt_malloc+0x27>
  40154f:	b8 00 00 00 00       	mov    $0x0,%eax
  401554:	5d                   	pop    %rbp
  401555:	c3                   	retq   

0000000000401556 <crt_brk>:
  401556:	55                   	push   %rbp
  401557:	48 89 e5             	mov    %rsp,%rbp
  40155a:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  40155e:	48 c7 45 f8 00 00 00 	movq   $0x0,-0x8(%rbp)
  401565:	00 
  401566:	48 c7 c0 2d 00 00 00 	mov    $0x2d,%rax
  40156d:	48 8b 5d e8          	mov    -0x18(%rbp),%rbx
  401571:	cd 80                	int    $0x80
  401573:	48 89 c0             	mov    %rax,%rax
  401576:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  40157a:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40157e:	5d                   	pop    %rbp
  40157f:	c3                   	retq   

0000000000401580 <crt_heap_init>:
  401580:	55                   	push   %rbp
  401581:	48 89 e5             	mov    %rsp,%rbp
  401584:	48 83 ec 20          	sub    $0x20,%rsp
  401588:	48 c7 45 f8 00 00 00 	movq   $0x0,-0x8(%rbp)
  40158f:	00 
  401590:	48 c7 45 f0 00 00 00 	movq   $0x0,-0x10(%rbp)
  401597:	00 
  401598:	c7 45 ec 10 27 00 00 	movl   $0x2710,-0x14(%rbp)
  40159f:	bf 00 00 00 00       	mov    $0x0,%edi
  4015a4:	e8 ad ff ff ff       	callq  401556 <crt_brk>
  4015a9:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  4015ad:	8b 55 ec             	mov    -0x14(%rbp),%edx
  4015b0:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4015b4:	48 01 d0             	add    %rdx,%rax
  4015b7:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
  4015bb:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  4015bf:	48 89 c7             	mov    %rax,%rdi
  4015c2:	e8 8f ff ff ff       	callq  401556 <crt_brk>
  4015c7:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
  4015cb:	8b 55 ec             	mov    -0x14(%rbp),%edx
  4015ce:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4015d2:	48 01 d0             	add    %rdx,%rax
  4015d5:	48 39 45 e0          	cmp    %rax,-0x20(%rbp)
  4015d9:	74 07                	je     4015e2 <crt_heap_init+0x62>
  4015db:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  4015e0:	eb 48                	jmp    40162a <crt_heap_init+0xaa>
  4015e2:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  4015e6:	48 89 45 f0          	mov    %rax,-0x10(%rbp)
  4015ea:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  4015ee:	8b 55 ec             	mov    -0x14(%rbp),%edx
  4015f1:	89 50 04             	mov    %edx,0x4(%rax)
  4015f4:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  4015f8:	c7 00 ab ab ab ab    	movl   $0xabababab,(%rax)
  4015fe:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  401602:	48 c7 40 10 00 00 00 	movq   $0x0,0x10(%rax)
  401609:	00 
  40160a:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  40160e:	48 8b 50 10          	mov    0x10(%rax),%rdx
  401612:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  401616:	48 89 50 08          	mov    %rdx,0x8(%rax)
  40161a:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  40161e:	48 89 05 fb 29 00 00 	mov    %rax,0x29fb(%rip)        # 404020 <list_head>
  401625:	b8 00 00 00 00       	mov    $0x0,%eax
  40162a:	c9                   	leaveq 
  40162b:	c3                   	retq   

000000000040162c <crt_fatal_error>:
  40162c:	55                   	push   %rbp
  40162d:	48 89 e5             	mov    %rsp,%rbp
  401630:	48 83 ec 10          	sub    $0x10,%rsp
  401634:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  401638:	bf 01 00 00 00       	mov    $0x1,%edi
  40163d:	e8 65 00 00 00       	callq  4016a7 <crt_exit>
  401642:	90                   	nop
  401643:	c9                   	leaveq 
  401644:	c3                   	retq   

0000000000401645 <mini_crt_entry>:
  401645:	55                   	push   %rbp
  401646:	48 89 e5             	mov    %rsp,%rbp
  401649:	48 83 ec 20          	sub    $0x20,%rsp
  40164d:	48 c7 45 f8 00 00 00 	movq   $0x0,-0x8(%rbp)
  401654:	00 
  401655:	48 89 e8             	mov    %rbp,%rax
  401658:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  40165c:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401660:	8b 40 08             	mov    0x8(%rax),%eax
  401663:	89 45 f4             	mov    %eax,-0xc(%rbp)
  401666:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40166a:	48 83 c0 10          	add    $0x10,%rax
  40166e:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
  401672:	b8 00 00 00 00       	mov    $0x0,%eax
  401677:	e8 04 ff ff ff       	callq  401580 <crt_heap_init>
  40167c:	85 c0                	test   %eax,%eax
  40167e:	74 0c                	je     40168c <mini_crt_entry+0x47>
  401680:	48 8d 3d 15 0a 00 00 	lea    0xa15(%rip),%rdi        # 40209c <etext+0x3e2>
  401687:	e8 a0 ff ff ff       	callq  40162c <crt_fatal_error>
  40168c:	48 8b 55 e8          	mov    -0x18(%rbp),%rdx
  401690:	8b 45 f4             	mov    -0xc(%rbp),%eax
  401693:	48 89 d6             	mov    %rdx,%rsi
  401696:	89 c7                	mov    %eax,%edi
  401698:	e8 c0 fb ff ff       	callq  40125d <main>
  40169d:	89 c7                	mov    %eax,%edi
  40169f:	e8 03 00 00 00       	callq  4016a7 <crt_exit>
  4016a4:	90                   	nop
  4016a5:	c9                   	leaveq 
  4016a6:	c3                   	retq   

00000000004016a7 <crt_exit>:
  4016a7:	55                   	push   %rbp
  4016a8:	48 89 e5             	mov    %rsp,%rbp
  4016ab:	89 7d fc             	mov    %edi,-0x4(%rbp)
  4016ae:	48 8b 5d fc          	mov    -0x4(%rbp),%rbx
  4016b2:	b8 01 00 00 00       	mov    $0x1,%eax
  4016b7:	cd 80                	int    $0x80
  4016b9:	f4                   	hlt    
  4016ba:	90                   	nop
  4016bb:	5d                   	pop    %rbp
  4016bc:	c3                   	retq   

00000000004016bd <puts>:
  4016bd:	55                   	push   %rbp
  4016be:	48 89 e5             	mov    %rsp,%rbp
  4016c1:	48 83 ec 20          	sub    $0x20,%rsp
  4016c5:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  4016c9:	48 89 75 e0          	mov    %rsi,-0x20(%rbp)
  4016cd:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  4016d1:	48 89 c7             	mov    %rax,%rdi
  4016d4:	e8 d4 04 00 00       	callq  401bad <strlen>
  4016d9:	89 45 fc             	mov    %eax,-0x4(%rbp)
  4016dc:	48 8b 4d e0          	mov    -0x20(%rbp),%rcx
  4016e0:	8b 55 fc             	mov    -0x4(%rbp),%edx
  4016e3:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  4016e7:	be 01 00 00 00       	mov    $0x1,%esi
  4016ec:	48 89 c7             	mov    %rax,%rdi
  4016ef:	e8 91 05 00 00       	callq  401c85 <fwrite>
  4016f4:	39 45 fc             	cmp    %eax,-0x4(%rbp)
  4016f7:	74 07                	je     401700 <puts+0x43>
  4016f9:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  4016fe:	eb 03                	jmp    401703 <puts+0x46>
  401700:	8b 45 fc             	mov    -0x4(%rbp),%eax
  401703:	c9                   	leaveq 
  401704:	c3                   	retq   

0000000000401705 <putc>:
  401705:	55                   	push   %rbp
  401706:	48 89 e5             	mov    %rsp,%rbp
  401709:	48 83 ec 10          	sub    $0x10,%rsp
  40170d:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  401711:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401715:	b9 01 00 00 00       	mov    $0x1,%ecx
  40171a:	ba 01 00 00 00       	mov    $0x1,%edx
  40171f:	be 01 00 00 00       	mov    $0x1,%esi
  401724:	48 89 c7             	mov    %rax,%rdi
  401727:	e8 59 05 00 00       	callq  401c85 <fwrite>
  40172c:	83 f8 01             	cmp    $0x1,%eax
  40172f:	74 07                	je     401738 <putc+0x33>
  401731:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  401736:	eb 05                	jmp    40173d <putc+0x38>
  401738:	b8 01 00 00 00       	mov    $0x1,%eax
  40173d:	c9                   	leaveq 
  40173e:	c3                   	retq   

000000000040173f <convert>:
  40173f:	55                   	push   %rbp
  401740:	48 89 e5             	mov    %rsp,%rbp
  401743:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  401747:	89 75 e4             	mov    %esi,-0x1c(%rbp)
  40174a:	48 8d 05 20 29 00 00 	lea    0x2920(%rip),%rax        # 404071 <buffer.1437+0x31>
  401751:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401755:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401759:	c6 00 00             	movb   $0x0,(%rax)
  40175c:	8b 45 e4             	mov    -0x1c(%rbp),%eax
  40175f:	48 63 c8             	movslq %eax,%rcx
  401762:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401766:	48 99                	cqto   
  401768:	48 f7 f9             	idiv   %rcx
  40176b:	48 83 6d f8 01       	subq   $0x1,-0x8(%rbp)
  401770:	48 8d 05 89 28 00 00 	lea    0x2889(%rip),%rax        # 404000 <Representation.1436>
  401777:	0f b6 14 02          	movzbl (%rdx,%rax,1),%edx
  40177b:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40177f:	88 10                	mov    %dl,(%rax)
  401781:	8b 45 e4             	mov    -0x1c(%rbp),%eax
  401784:	48 63 f0             	movslq %eax,%rsi
  401787:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  40178b:	48 99                	cqto   
  40178d:	48 f7 fe             	idiv   %rsi
  401790:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
  401794:	48 83 7d e8 00       	cmpq   $0x0,-0x18(%rbp)
  401799:	75 c1                	jne    40175c <convert+0x1d>
  40179b:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  40179f:	5d                   	pop    %rbp
  4017a0:	c3                   	retq   

00000000004017a1 <_puts>:
  4017a1:	55                   	push   %rbp
  4017a2:	48 89 e5             	mov    %rsp,%rbp
  4017a5:	48 83 ec 20          	sub    $0x20,%rsp
  4017a9:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  4017ad:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%rbp)
  4017b4:	eb 15                	jmp    4017cb <_puts+0x2a>
  4017b6:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  4017ba:	48 89 c7             	mov    %rax,%rdi
  4017bd:	e8 43 ff ff ff       	callq  401705 <putc>
  4017c2:	48 83 45 e8 01       	addq   $0x1,-0x18(%rbp)
  4017c7:	83 45 fc 01          	addl   $0x1,-0x4(%rbp)
  4017cb:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  4017cf:	0f b6 00             	movzbl (%rax),%eax
  4017d2:	84 c0                	test   %al,%al
  4017d4:	75 e0                	jne    4017b6 <_puts+0x15>
  4017d6:	8b 45 fc             	mov    -0x4(%rbp),%eax
  4017d9:	c9                   	leaveq 
  4017da:	c3                   	retq   

00000000004017db <printf>:
  4017db:	55                   	push   %rbp
  4017dc:	48 89 e5             	mov    %rsp,%rbp
  4017df:	48 81 ec 00 01 00 00 	sub    $0x100,%rsp
  4017e6:	48 89 bd 08 ff ff ff 	mov    %rdi,-0xf8(%rbp)
  4017ed:	48 89 b5 58 ff ff ff 	mov    %rsi,-0xa8(%rbp)
  4017f4:	48 89 95 60 ff ff ff 	mov    %rdx,-0xa0(%rbp)
  4017fb:	48 89 8d 68 ff ff ff 	mov    %rcx,-0x98(%rbp)
  401802:	4c 89 85 70 ff ff ff 	mov    %r8,-0x90(%rbp)
  401809:	4c 89 8d 78 ff ff ff 	mov    %r9,-0x88(%rbp)
  401810:	84 c0                	test   %al,%al
  401812:	74 20                	je     401834 <printf+0x59>
  401814:	0f 29 45 80          	movaps %xmm0,-0x80(%rbp)
  401818:	0f 29 4d 90          	movaps %xmm1,-0x70(%rbp)
  40181c:	0f 29 55 a0          	movaps %xmm2,-0x60(%rbp)
  401820:	0f 29 5d b0          	movaps %xmm3,-0x50(%rbp)
  401824:	0f 29 65 c0          	movaps %xmm4,-0x40(%rbp)
  401828:	0f 29 6d d0          	movaps %xmm5,-0x30(%rbp)
  40182c:	0f 29 75 e0          	movaps %xmm6,-0x20(%rbp)
  401830:	0f 29 7d f0          	movaps %xmm7,-0x10(%rbp)
  401834:	c7 85 44 ff ff ff 00 	movl   $0x0,-0xbc(%rbp)
  40183b:	00 00 00 
  40183e:	c7 85 18 ff ff ff 08 	movl   $0x8,-0xe8(%rbp)
  401845:	00 00 00 
  401848:	c7 85 1c ff ff ff 30 	movl   $0x30,-0xe4(%rbp)
  40184f:	00 00 00 
  401852:	48 8d 45 10          	lea    0x10(%rbp),%rax
  401856:	48 89 85 20 ff ff ff 	mov    %rax,-0xe0(%rbp)
  40185d:	48 8d 85 50 ff ff ff 	lea    -0xb0(%rbp),%rax
  401864:	48 89 85 28 ff ff ff 	mov    %rax,-0xd8(%rbp)
  40186b:	48 8b 85 08 ff ff ff 	mov    -0xf8(%rbp),%rax
  401872:	48 89 85 48 ff ff ff 	mov    %rax,-0xb8(%rbp)
  401879:	e9 12 03 00 00       	jmpq   401b90 <printf+0x3b5>
  40187e:	48 8b 85 48 ff ff ff 	mov    -0xb8(%rbp),%rax
  401885:	48 89 c7             	mov    %rax,%rdi
  401888:	e8 78 fe ff ff       	callq  401705 <putc>
  40188d:	83 85 44 ff ff ff 01 	addl   $0x1,-0xbc(%rbp)
  401894:	48 83 85 48 ff ff ff 	addq   $0x1,-0xb8(%rbp)
  40189b:	01 
  40189c:	48 8b 85 48 ff ff ff 	mov    -0xb8(%rbp),%rax
  4018a3:	0f b6 00             	movzbl (%rax),%eax
  4018a6:	3c 25                	cmp    $0x25,%al
  4018a8:	74 0e                	je     4018b8 <printf+0xdd>
  4018aa:	48 8b 85 48 ff ff ff 	mov    -0xb8(%rbp),%rax
  4018b1:	0f b6 00             	movzbl (%rax),%eax
  4018b4:	84 c0                	test   %al,%al
  4018b6:	75 c6                	jne    40187e <printf+0xa3>
  4018b8:	48 8b 85 48 ff ff ff 	mov    -0xb8(%rbp),%rax
  4018bf:	0f b6 00             	movzbl (%rax),%eax
  4018c2:	84 c0                	test   %al,%al
  4018c4:	0f 84 da 02 00 00    	je     401ba4 <printf+0x3c9>
  4018ca:	48 83 85 48 ff ff ff 	addq   $0x1,-0xb8(%rbp)
  4018d1:	01 
  4018d2:	48 8b 85 48 ff ff ff 	mov    -0xb8(%rbp),%rax
  4018d9:	0f b6 00             	movzbl (%rax),%eax
  4018dc:	0f be c0             	movsbl %al,%eax
  4018df:	83 e8 63             	sub    $0x63,%eax
  4018e2:	83 f8 15             	cmp    $0x15,%eax
  4018e5:	0f 87 9d 02 00 00    	ja     401b88 <printf+0x3ad>
  4018eb:	89 c0                	mov    %eax,%eax
  4018ed:	48 8d 14 85 00 00 00 	lea    0x0(,%rax,4),%rdx
  4018f4:	00 
  4018f5:	48 8d 05 b4 07 00 00 	lea    0x7b4(%rip),%rax        # 4020b0 <etext+0x3f6>
  4018fc:	8b 04 02             	mov    (%rdx,%rax,1),%eax
  4018ff:	48 98                	cltq   
  401901:	48 8d 15 a8 07 00 00 	lea    0x7a8(%rip),%rdx        # 4020b0 <etext+0x3f6>
  401908:	48 01 d0             	add    %rdx,%rax
  40190b:	ff e0                	jmpq   *%rax
  40190d:	8b 85 18 ff ff ff    	mov    -0xe8(%rbp),%eax
  401913:	83 f8 2f             	cmp    $0x2f,%eax
  401916:	77 23                	ja     40193b <printf+0x160>
  401918:	48 8b 85 28 ff ff ff 	mov    -0xd8(%rbp),%rax
  40191f:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401925:	89 d2                	mov    %edx,%edx
  401927:	48 01 d0             	add    %rdx,%rax
  40192a:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401930:	83 c2 08             	add    $0x8,%edx
  401933:	89 95 18 ff ff ff    	mov    %edx,-0xe8(%rbp)
  401939:	eb 12                	jmp    40194d <printf+0x172>
  40193b:	48 8b 85 20 ff ff ff 	mov    -0xe0(%rbp),%rax
  401942:	48 8d 50 08          	lea    0x8(%rax),%rdx
  401946:	48 89 95 20 ff ff ff 	mov    %rdx,-0xe0(%rbp)
  40194d:	8b 00                	mov    (%rax),%eax
  40194f:	88 85 37 ff ff ff    	mov    %al,-0xc9(%rbp)
  401955:	48 8d 85 37 ff ff ff 	lea    -0xc9(%rbp),%rax
  40195c:	48 89 c7             	mov    %rax,%rdi
  40195f:	e8 a1 fd ff ff       	callq  401705 <putc>
  401964:	83 85 44 ff ff ff 01 	addl   $0x1,-0xbc(%rbp)
  40196b:	e9 18 02 00 00       	jmpq   401b88 <printf+0x3ad>
  401970:	8b 85 18 ff ff ff    	mov    -0xe8(%rbp),%eax
  401976:	83 f8 2f             	cmp    $0x2f,%eax
  401979:	77 23                	ja     40199e <printf+0x1c3>
  40197b:	48 8b 85 28 ff ff ff 	mov    -0xd8(%rbp),%rax
  401982:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401988:	89 d2                	mov    %edx,%edx
  40198a:	48 01 d0             	add    %rdx,%rax
  40198d:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401993:	83 c2 08             	add    $0x8,%edx
  401996:	89 95 18 ff ff ff    	mov    %edx,-0xe8(%rbp)
  40199c:	eb 12                	jmp    4019b0 <printf+0x1d5>
  40199e:	48 8b 85 20 ff ff ff 	mov    -0xe0(%rbp),%rax
  4019a5:	48 8d 50 08          	lea    0x8(%rax),%rdx
  4019a9:	48 89 95 20 ff ff ff 	mov    %rdx,-0xe0(%rbp)
  4019b0:	8b 00                	mov    (%rax),%eax
  4019b2:	89 85 40 ff ff ff    	mov    %eax,-0xc0(%rbp)
  4019b8:	8b 85 40 ff ff ff    	mov    -0xc0(%rbp),%eax
  4019be:	be 0a 00 00 00       	mov    $0xa,%esi
  4019c3:	48 89 c7             	mov    %rax,%rdi
  4019c6:	e8 74 fd ff ff       	callq  40173f <convert>
  4019cb:	48 89 c7             	mov    %rax,%rdi
  4019ce:	e8 ce fd ff ff       	callq  4017a1 <_puts>
  4019d3:	01 85 44 ff ff ff    	add    %eax,-0xbc(%rbp)
  4019d9:	e9 aa 01 00 00       	jmpq   401b88 <printf+0x3ad>
  4019de:	8b 85 18 ff ff ff    	mov    -0xe8(%rbp),%eax
  4019e4:	83 f8 2f             	cmp    $0x2f,%eax
  4019e7:	77 23                	ja     401a0c <printf+0x231>
  4019e9:	48 8b 85 28 ff ff ff 	mov    -0xd8(%rbp),%rax
  4019f0:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  4019f6:	89 d2                	mov    %edx,%edx
  4019f8:	48 01 d0             	add    %rdx,%rax
  4019fb:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401a01:	83 c2 08             	add    $0x8,%edx
  401a04:	89 95 18 ff ff ff    	mov    %edx,-0xe8(%rbp)
  401a0a:	eb 12                	jmp    401a1e <printf+0x243>
  401a0c:	48 8b 85 20 ff ff ff 	mov    -0xe0(%rbp),%rax
  401a13:	48 8d 50 08          	lea    0x8(%rax),%rdx
  401a17:	48 89 95 20 ff ff ff 	mov    %rdx,-0xe0(%rbp)
  401a1e:	8b 00                	mov    (%rax),%eax
  401a20:	89 85 40 ff ff ff    	mov    %eax,-0xc0(%rbp)
  401a26:	8b 85 40 ff ff ff    	mov    -0xc0(%rbp),%eax
  401a2c:	be 08 00 00 00       	mov    $0x8,%esi
  401a31:	48 89 c7             	mov    %rax,%rdi
  401a34:	e8 06 fd ff ff       	callq  40173f <convert>
  401a39:	48 89 c7             	mov    %rax,%rdi
  401a3c:	e8 60 fd ff ff       	callq  4017a1 <_puts>
  401a41:	01 85 44 ff ff ff    	add    %eax,-0xbc(%rbp)
  401a47:	e9 3c 01 00 00       	jmpq   401b88 <printf+0x3ad>
  401a4c:	8b 85 18 ff ff ff    	mov    -0xe8(%rbp),%eax
  401a52:	83 f8 2f             	cmp    $0x2f,%eax
  401a55:	77 23                	ja     401a7a <printf+0x29f>
  401a57:	48 8b 85 28 ff ff ff 	mov    -0xd8(%rbp),%rax
  401a5e:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401a64:	89 d2                	mov    %edx,%edx
  401a66:	48 01 d0             	add    %rdx,%rax
  401a69:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401a6f:	83 c2 08             	add    $0x8,%edx
  401a72:	89 95 18 ff ff ff    	mov    %edx,-0xe8(%rbp)
  401a78:	eb 12                	jmp    401a8c <printf+0x2b1>
  401a7a:	48 8b 85 20 ff ff ff 	mov    -0xe0(%rbp),%rax
  401a81:	48 8d 50 08          	lea    0x8(%rax),%rdx
  401a85:	48 89 95 20 ff ff ff 	mov    %rdx,-0xe0(%rbp)
  401a8c:	48 8b 00             	mov    (%rax),%rax
  401a8f:	48 89 85 38 ff ff ff 	mov    %rax,-0xc8(%rbp)
  401a96:	48 8b 85 38 ff ff ff 	mov    -0xc8(%rbp),%rax
  401a9d:	48 89 c7             	mov    %rax,%rdi
  401aa0:	e8 fc fc ff ff       	callq  4017a1 <_puts>
  401aa5:	01 85 44 ff ff ff    	add    %eax,-0xbc(%rbp)
  401aab:	e9 d8 00 00 00       	jmpq   401b88 <printf+0x3ad>
  401ab0:	8b 85 18 ff ff ff    	mov    -0xe8(%rbp),%eax
  401ab6:	83 f8 2f             	cmp    $0x2f,%eax
  401ab9:	77 23                	ja     401ade <printf+0x303>
  401abb:	48 8b 85 28 ff ff ff 	mov    -0xd8(%rbp),%rax
  401ac2:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401ac8:	89 d2                	mov    %edx,%edx
  401aca:	48 01 d0             	add    %rdx,%rax
  401acd:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401ad3:	83 c2 08             	add    $0x8,%edx
  401ad6:	89 95 18 ff ff ff    	mov    %edx,-0xe8(%rbp)
  401adc:	eb 12                	jmp    401af0 <printf+0x315>
  401ade:	48 8b 85 20 ff ff ff 	mov    -0xe0(%rbp),%rax
  401ae5:	48 8d 50 08          	lea    0x8(%rax),%rdx
  401ae9:	48 89 95 20 ff ff ff 	mov    %rdx,-0xe0(%rbp)
  401af0:	48 8b 00             	mov    (%rax),%rax
  401af3:	48 89 85 38 ff ff ff 	mov    %rax,-0xc8(%rbp)
  401afa:	48 8b 85 38 ff ff ff 	mov    -0xc8(%rbp),%rax
  401b01:	be 10 00 00 00       	mov    $0x10,%esi
  401b06:	48 89 c7             	mov    %rax,%rdi
  401b09:	e8 31 fc ff ff       	callq  40173f <convert>
  401b0e:	48 89 c7             	mov    %rax,%rdi
  401b11:	e8 8b fc ff ff       	callq  4017a1 <_puts>
  401b16:	01 85 44 ff ff ff    	add    %eax,-0xbc(%rbp)
  401b1c:	eb 6a                	jmp    401b88 <printf+0x3ad>
  401b1e:	8b 85 18 ff ff ff    	mov    -0xe8(%rbp),%eax
  401b24:	83 f8 2f             	cmp    $0x2f,%eax
  401b27:	77 23                	ja     401b4c <printf+0x371>
  401b29:	48 8b 85 28 ff ff ff 	mov    -0xd8(%rbp),%rax
  401b30:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401b36:	89 d2                	mov    %edx,%edx
  401b38:	48 01 d0             	add    %rdx,%rax
  401b3b:	8b 95 18 ff ff ff    	mov    -0xe8(%rbp),%edx
  401b41:	83 c2 08             	add    $0x8,%edx
  401b44:	89 95 18 ff ff ff    	mov    %edx,-0xe8(%rbp)
  401b4a:	eb 12                	jmp    401b5e <printf+0x383>
  401b4c:	48 8b 85 20 ff ff ff 	mov    -0xe0(%rbp),%rax
  401b53:	48 8d 50 08          	lea    0x8(%rax),%rdx
  401b57:	48 89 95 20 ff ff ff 	mov    %rdx,-0xe0(%rbp)
  401b5e:	8b 00                	mov    (%rax),%eax
  401b60:	89 85 40 ff ff ff    	mov    %eax,-0xc0(%rbp)
  401b66:	8b 85 40 ff ff ff    	mov    -0xc0(%rbp),%eax
  401b6c:	be 10 00 00 00       	mov    $0x10,%esi
  401b71:	48 89 c7             	mov    %rax,%rdi
  401b74:	e8 c6 fb ff ff       	callq  40173f <convert>
  401b79:	48 89 c7             	mov    %rax,%rdi
  401b7c:	e8 20 fc ff ff       	callq  4017a1 <_puts>
  401b81:	01 85 44 ff ff ff    	add    %eax,-0xbc(%rbp)
  401b87:	90                   	nop
  401b88:	48 83 85 48 ff ff ff 	addq   $0x1,-0xb8(%rbp)
  401b8f:	01 
  401b90:	48 8b 85 48 ff ff ff 	mov    -0xb8(%rbp),%rax
  401b97:	0f b6 00             	movzbl (%rax),%eax
  401b9a:	84 c0                	test   %al,%al
  401b9c:	0f 85 fa fc ff ff    	jne    40189c <printf+0xc1>
  401ba2:	eb 01                	jmp    401ba5 <printf+0x3ca>
  401ba4:	90                   	nop
  401ba5:	8b 85 44 ff ff ff    	mov    -0xbc(%rbp),%eax
  401bab:	c9                   	leaveq 
  401bac:	c3                   	retq   

0000000000401bad <strlen>:
  401bad:	55                   	push   %rbp
  401bae:	48 89 e5             	mov    %rsp,%rbp
  401bb1:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  401bb5:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%rbp)
  401bbc:	48 83 7d e8 00       	cmpq   $0x0,-0x18(%rbp)
  401bc1:	75 10                	jne    401bd3 <strlen+0x26>
  401bc3:	b8 00 00 00 00       	mov    $0x0,%eax
  401bc8:	eb 17                	jmp    401be1 <strlen+0x34>
  401bca:	83 45 fc 01          	addl   $0x1,-0x4(%rbp)
  401bce:	48 83 45 e8 01       	addq   $0x1,-0x18(%rbp)
  401bd3:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401bd7:	0f b6 00             	movzbl (%rax),%eax
  401bda:	84 c0                	test   %al,%al
  401bdc:	75 ec                	jne    401bca <strlen+0x1d>
  401bde:	8b 45 fc             	mov    -0x4(%rbp),%eax
  401be1:	5d                   	pop    %rbp
  401be2:	c3                   	retq   

0000000000401be3 <strcpy>:
  401be3:	55                   	push   %rbp
  401be4:	48 89 e5             	mov    %rsp,%rbp
  401be7:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  401beb:	48 89 75 e0          	mov    %rsi,-0x20(%rbp)
  401bef:	48 83 7d e8 00       	cmpq   $0x0,-0x18(%rbp)
  401bf4:	75 07                	jne    401bfd <strcpy+0x1a>
  401bf6:	b8 00 00 00 00       	mov    $0x0,%eax
  401bfb:	eb 37                	jmp    401c34 <strcpy+0x51>
  401bfd:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401c01:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401c05:	eb 17                	jmp    401c1e <strcpy+0x3b>
  401c07:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  401c0b:	0f b6 10             	movzbl (%rax),%edx
  401c0e:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401c12:	88 10                	mov    %dl,(%rax)
  401c14:	48 83 45 e8 01       	addq   $0x1,-0x18(%rbp)
  401c19:	48 83 45 e0 01       	addq   $0x1,-0x20(%rbp)
  401c1e:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  401c22:	0f b6 00             	movzbl (%rax),%eax
  401c25:	84 c0                	test   %al,%al
  401c27:	75 de                	jne    401c07 <strcpy+0x24>
  401c29:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401c2d:	c6 00 00             	movb   $0x0,(%rax)
  401c30:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401c34:	5d                   	pop    %rbp
  401c35:	c3                   	retq   

0000000000401c36 <write>:
  401c36:	55                   	push   %rbp
  401c37:	48 89 e5             	mov    %rsp,%rbp
  401c3a:	48 83 ec 30          	sub    $0x30,%rsp
  401c3e:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
  401c42:	48 89 75 e0          	mov    %rsi,-0x20(%rbp)
  401c46:	48 89 55 d8          	mov    %rdx,-0x28(%rbp)
  401c4a:	48 c7 45 f8 00 00 00 	movq   $0x0,-0x8(%rbp)
  401c51:	00 
  401c52:	48 c7 c0 04 00 00 00 	mov    $0x4,%rax
  401c59:	48 8b 5d e8          	mov    -0x18(%rbp),%rbx
  401c5d:	48 8b 4d e0          	mov    -0x20(%rbp),%rcx
  401c61:	48 8b 55 d8          	mov    -0x28(%rbp),%rdx
  401c65:	cd 80                	int    $0x80
  401c67:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  401c6b:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401c6f:	48 83 f8 f2          	cmp    $0xfffffffffffffff2,%rax
  401c73:	75 0a                	jne    401c7f <write+0x49>
  401c75:	bf 0e 00 00 00       	mov    $0xe,%edi
  401c7a:	e8 28 fa ff ff       	callq  4016a7 <crt_exit>
  401c7f:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  401c83:	c9                   	leaveq 
  401c84:	c3                   	retq   

0000000000401c85 <fwrite>:
  401c85:	55                   	push   %rbp
  401c86:	48 89 e5             	mov    %rsp,%rbp
  401c89:	48 83 ec 20          	sub    $0x20,%rsp
  401c8d:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  401c91:	89 75 f4             	mov    %esi,-0xc(%rbp)
  401c94:	89 55 f0             	mov    %edx,-0x10(%rbp)
  401c97:	48 89 4d e8          	mov    %rcx,-0x18(%rbp)
  401c9b:	8b 45 f4             	mov    -0xc(%rbp),%eax
  401c9e:	0f af 45 f0          	imul   -0x10(%rbp),%eax
  401ca2:	48 63 d0             	movslq %eax,%rdx
  401ca5:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  401ca9:	48 8b 4d f8          	mov    -0x8(%rbp),%rcx
  401cad:	48 89 ce             	mov    %rcx,%rsi
  401cb0:	48 89 c7             	mov    %rax,%rdi
  401cb3:	e8 7e ff ff ff       	callq  401c36 <write>
  401cb8:	c9                   	leaveq 
  401cb9:	c3                   	retq   
