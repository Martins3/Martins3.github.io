/*
 * # aarch64 midr mpidr
 * <!-- 4fe3640a-2dfe-4fc1-8f96-6a4264b43da6 -->
 * 用来区分是哪一个 CPU
 * - MIDR_EL1（Main ID Register）：标识处理器的厂商、架构、主/次版本等，相当于 CPU 的“身份证”。
 * - MPIDR_EL1（Multiprocessor Affinity Register）：用于描述当前 CPU 在多核/多簇系统中的拓扑位置（如 cluster、core、thread ID）
 *
 * 读取并解码 AArch64 MIDR_EL1 / MPIDR_EL1 寄存器。
 *
 * mpidr 在 code/src/m/arch/aarch64/aarch64.c 中也有测试，从内核中读结果就是完全预期的。
 *
 * Kunpeng-920 输出:
 *
 * === MIDR_EL1 (Main ID Register) ===
 * Raw: 0x00000000481fd010
 * Implementer:          0x48 (HiSilicon)
 * Variant:              0x1 (Major revision)
 * Architecture:         0xf (ARMv8-A or later)
 * Part Number:          0xd01 (Kunpeng-920 / tsv110)
 * Revision:             0x0 (Minor revision)
 *
 * === MPIDR_EL1 (Multiprocessor Affinity Register) ===
 * Raw: 0x0000000080000000
 * Multiprocessor:       Yes
 * Uniprocessor:         No
 * Multi-threading:      Not supported
 * Topology:
 *   Aff0 (Core/Thread): 0
 * Logical CPU ID:       Core=0
 *
 * 在 Apple Silicon (mac) 上:
 *
 * === MIDR_EL1 (Main ID Register) ===
 * Raw: 0x00000000611f0320
 * Implementer:          0x61 (Apple)
 * Variant:              0x1 (Major revision)
 * Architecture:         0xf (ARMv8-A or later)
 * Part Number:          0x032 (Apple Silicon core)
 * Revision:             0x0 (Minor revision)
 *
 * === MPIDR_EL1 (Multiprocessor Affinity Register) ===
 * Raw: 0x0000000080000000
 * Multiprocessor:       Yes
 * Uniprocessor:         No
 * Multi-threading:      Not supported
 * Topology:
 *   Aff0 (Core/Thread): 0
 * Logical CPU ID:       Core=0
 */
#include <stdint.h>
#include <stdio.h>

// Extract bits [high:low] from val
static inline uint64_t bits(uint64_t val, int high, int low) {
  return (val >> low) & ((1ULL << (high - low + 1)) - 1);
}

// Decode MIDR_EL1
static void decode_midr(uint64_t midr) {
  printf("\n=== MIDR_EL1 (Main ID Register) ===\n");
  printf("Raw: 0x%016lx\n", midr);

  // ARM MIDR_EL1 layout:
  // [3:0]   Revision
  // [15:4]  PartNum
  // [19:16] Architecture
  // [23:20] Variant
  // [31:24] Implementer
  uint32_t revision = bits(midr, 3, 0);
  uint32_t part_num = bits(midr, 15, 4);
  uint32_t architecture = bits(midr, 19, 16);
  uint32_t variant = bits(midr, 23, 20);
  uint32_t implementer = bits(midr, 31, 24);

  printf("Implementer:          0x%02x", implementer);
  switch (implementer) {
  case 0x41:
    printf(" (ARM Ltd)\n");
    break;
  case 0x42:
    printf(" (Broadcom)\n");
    break;
  case 0x43:
    printf(" (Cavium)\n");
    break;
  case 0x44:
    printf(" (DEC)\n");
    break;
  case 0x46:
    printf(" (Fujitsu)\n");
    break;
  case 0x48:
    printf(" (HiSilicon)\n");
    break;
  case 0x49:
    printf(" (Infineon)\n");
    break;
  case 0x4D:
    printf(" (Motorola)\n");
    break;
  case 0x4E:
    printf(" (NVIDIA)\n");
    break;
  case 0x50:
    printf(" (APM / Ampere)\n");
    break;
  case 0x51:
    printf(" (Qualcomm)\n");
    break;
  case 0x53:
    printf(" (Samsung)\n");
    break;
  case 0x56:
    printf(" (Marvell)\n");
    break;
  case 0x61:
    printf(" (Apple)\n");
    break;
  case 0x63:
    printf(" (Cirrus Logic)\n");
    break;
  case 0x66:
    printf(" (Faraday)\n");
    break;
  case 0x69:
    printf(" (Intel)\n");
    break;
  case 0x6D:
    printf(" (Microchip)\n");
    break;
  case 0x70:
    printf(" (Phytium)\n");
    break;
  default:
    printf(" (Unknown)\n");
    break;
  }

  printf("Variant:              0x%x (Major revision)\n", variant);
  printf("Architecture:         0x%x", architecture);
  if (architecture == 0xF) {
    printf(" (ARMv8-A or later)\n");
  } else if (architecture == 0x0) {
    printf(" (Pre-ARMv8, derived from PartNum)\n");
  } else {
    printf(" (ARMv%x)\n", architecture);
  }

  printf("Part Number:          0x%03x", part_num);
  // Decode common part numbers
  if (implementer == 0x41) { // ARM Ltd
    switch (part_num) {
    case 0xD01:
      printf(" (Cortex-A32)\n");
      break;
    case 0xD02:
      printf(" (Cortex-A34)\n");
      break;
    case 0xD03:
      printf(" (Cortex-A53)\n");
      break;
    case 0xD04:
      printf(" (Cortex-A35)\n");
      break;
    case 0xD05:
      printf(" (Cortex-A55)\n");
      break;
    case 0xD06:
      printf(" (Cortex-A65)\n");
      break;
    case 0xD07:
      printf(" (Cortex-A57)\n");
      break;
    case 0xD08:
      printf(" (Cortex-A72)\n");
      break;
    case 0xD09:
      printf(" (Cortex-A73)\n");
      break;
    case 0xD0A:
      printf(" (Cortex-A75)\n");
      break;
    case 0xD0B:
      printf(" (Cortex-A76)\n");
      break;
    case 0xD0C:
      printf(" (Neoverse N1)\n");
      break;
    case 0xD0D:
      printf(" (Cortex-A77)\n");
      break;
    case 0xD0E:
      printf(" (Cortex-A76AE)\n");
      break;
    case 0xD40:
      printf(" (Neoverse V1)\n");
      break;
    case 0xD41:
      printf(" (Cortex-A78)\n");
      break;
    case 0xD42:
      printf(" (Cortex-A78AE)\n");
      break;
    case 0xD43:
      printf(" (Cortex-A65AE)\n");
      break;
    case 0xD44:
      printf(" (Cortex-X1)\n");
      break;
    case 0xD46:
      printf(" (Cortex-A510)\n");
      break;
    case 0xD47:
      printf(" (Cortex-A710)\n");
      break;
    case 0xD48:
      printf(" (Cortex-X2)\n");
      break;
    case 0xD49:
      printf(" (Neoverse N2)\n");
      break;
    case 0xD4A:
      printf(" (Neoverse E1)\n");
      break;
    case 0xD4B:
      printf(" (Cortex-A715)\n");
      break;
    case 0xD4C:
      printf(" (Cortex-X3)\n");
      break;
    case 0xD4D:
      printf(" (Cortex-A520)\n");
      break;
    case 0xD4E:
      printf(" (Cortex-A720)\n");
      break;
    case 0xD4F:
      printf(" (Cortex-X4)\n");
      break;
    case 0xD80:
      printf(" (Neoverse V2)\n");
      break;
    default:
      printf(" (Unknown ARM core)\n");
      break;
    }
  } else if (implementer == 0x51) { // Qualcomm
    switch (part_num) {
    case 0x800:
      printf(" (Kryo 2xx Gold / Cortex-A73 derivative)\n");
      break;
    case 0x801:
      printf(" (Kryo 2xx Silver / Cortex-A53 derivative)\n");
      break;
    case 0x802:
      printf(" (Kryo 3xx Gold / Cortex-A75 derivative)\n");
      break;
    case 0x803:
      printf(" (Kryo 3xx Silver / Cortex-A55 derivative)\n");
      break;
    case 0x804:
      printf(" (Kryo 4xx Gold / Cortex-A76 derivative)\n");
      break;
    case 0x805:
      printf(" (Kryo 4xx Silver / Cortex-A55 derivative)\n");
      break;
    default:
      printf(" (Qualcomm Kryo or custom core)\n");
      break;
    }
  } else if (implementer == 0x46) { // Fujitsu
    if (part_num == 0x001)
      printf(" (A64FX)\n");
    else
      printf(" (Fujitsu custom core)\n");
  } else if (implementer == 0x48) { // HiSilicon
    switch (part_num) {
    case 0xD01:
      printf(" (Kunpeng-920 / tsv110)\n");
      break;
    default:
      printf(" (HiSilicon custom core)\n");
      break;
    }
  } else if (implementer == 0x61) { // Apple
    printf(" (Apple Silicon core)\n");
  } else {
    printf(" (Custom or unknown core)\n");
  }

  printf("Revision:             0x%x (Minor revision)\n", revision);
}

// Decode MPIDR_EL1
static void decode_mpidr(uint64_t mpidr) {
  printf("\n=== MPIDR_EL1 (Multiprocessor Affinity Register) ===\n");
  printf("Raw: 0x%016lx\n", mpidr);

  // Check if multiprocessor extensions are supported
  if (!(mpidr & (1ULL << 31))) {
    printf("Multiprocessor extensions not implemented.\n");
    return;
  }

  // ARM MPIDR_EL1 layout (ARMv8):
  // [7:0]   Aff0 (thread / core)
  // [15:8]  Aff1 (cluster / processor)
  // [23:16] Aff2 (higher cluster / die)
  // [24]    MT (multi-threading)
  // [30]    U (uniprocessor)
  // [31]    ME (multiprocessor extensions)
  // [39:32] Aff3 (ARMv8.0+)
  uint32_t aff0 = bits(mpidr, 7, 0);
  uint32_t aff1 = bits(mpidr, 15, 8);
  uint32_t aff2 = bits(mpidr, 23, 16);
  uint32_t aff3 = bits(mpidr, 39, 32);

  int mt = (mpidr >> 24) & 0x1;
  int u = (mpidr >> 30) & 0x1;

  printf("Multiprocessor:       Yes\n");
  printf("Uniprocessor:         %s\n", u ? "Yes" : "No");
  if (mt) {
    printf("Multi-threading:      Supported (SMT/hyperthreading)\n");
  } else {
    printf("Multi-threading:      Not supported\n");
  }

  // Determine highest affinity level populated
  int highest_aff = 0;
  if (aff3)
    highest_aff = 3;
  else if (aff2)
    highest_aff = 2;
  else if (aff1)
    highest_aff = 1;
  else
    highest_aff = 0;

  printf("Topology:\n");
  if (highest_aff >= 3)
    printf("  Aff3 (Die/Package): %u\n", aff3);
  if (highest_aff >= 2)
    printf("  Aff2 (Cluster):     %u\n", aff2);
  if (highest_aff >= 1)
    printf("  Aff1 (Sub-cluster): %u\n", aff1);
  printf("  Aff0 (Core/Thread): %u\n", aff0);

  // Human-readable CPU ID
  if (highest_aff >= 3) {
    printf("Logical CPU ID:       Die=%u, Cluster=%u, Core=%u\n", aff3, aff2,
           aff0);
  } else if (highest_aff >= 2) {
    printf("Logical CPU ID:       Cluster=%u, Core=%u\n", aff2, aff0);
  } else if (highest_aff >= 1) {
    printf("Logical CPU ID:       Sub-cluster=%u, Core=%u\n", aff1, aff0);
  } else {
    printf("Logical CPU ID:       Core=%u\n", aff0);
  }
}

int main(void) {
  uint64_t midr, mpidr;

  // Read MIDR_EL1
  asm volatile("mrs %0, MIDR_EL1" : "=r"(midr));

  // Read MPIDR_EL1
  asm volatile("mrs %0, MPIDR_EL1" : "=r"(mpidr));

  decode_midr(midr);
  decode_mpidr(mpidr);

  return 0;
}
