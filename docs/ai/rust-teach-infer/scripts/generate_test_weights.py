#!/usr/bin/env python3
"""
Generate test weights for Qwen2 model (for framework testing)

Usage:
    python generate_test_weights.py --config models/dev-qwen2-fixture/config.json --output models/Qwen2.5-0.5B/model.safetensors
"""

import json
import argparse
from pathlib import Path

try:
    import numpy as np
    from safetensors.numpy import save_file
except ImportError:
    print("Error: Please install required packages:")
    print("  pip install numpy safetensors")
    exit(1)


def generate_weights(config_path: Path, output_path: Path):
    """Generate random test weights based on config"""
    
    with open(config_path) as f:
        config = json.load(f)
    
    hidden_size = config["hidden_size"]
    intermediate_size = config["intermediate_size"]
    vocab_size = config["vocab_size"]
    num_layers = config["num_hidden_layers"]
    num_heads = config["num_attention_heads"]
    num_kv_heads = config.get("num_key_value_heads", num_heads)
    head_dim = hidden_size // num_heads
    
    tensors = {}
    
    # Embedding
    tensors["model.embed_tokens.weight"] = np.random.randn(vocab_size, hidden_size).astype(np.float32) * 0.02
    
    # LM Head
    tensors["lm_head.weight"] = np.random.randn(vocab_size, hidden_size).astype(np.float32) * 0.02
    
    # Layers
    for i in range(num_layers):
        prefix = f"model.layers.{i}"
        
        # Attention
        tensors[f"{prefix}.self_attn.q_proj.weight"] = np.random.randn(hidden_size, num_heads * head_dim).astype(np.float32) * 0.02
        tensors[f"{prefix}.self_attn.k_proj.weight"] = np.random.randn(hidden_size, num_kv_heads * head_dim).astype(np.float32) * 0.02
        tensors[f"{prefix}.self_attn.v_proj.weight"] = np.random.randn(hidden_size, num_kv_heads * head_dim).astype(np.float32) * 0.02
        tensors[f"{prefix}.self_attn.o_proj.weight"] = np.random.randn(num_heads * head_dim, hidden_size).astype(np.float32) * 0.02
        
        # MLP
        tensors[f"{prefix}.mlp.gate_proj.weight"] = np.random.randn(hidden_size, intermediate_size).astype(np.float32) * 0.02
        tensors[f"{prefix}.mlp.up_proj.weight"] = np.random.randn(hidden_size, intermediate_size).astype(np.float32) * 0.02
        tensors[f"{prefix}.mlp.down_proj.weight"] = np.random.randn(intermediate_size, hidden_size).astype(np.float32) * 0.02
        
        # Layer Norm
        tensors[f"{prefix}.input_layernorm.weight"] = np.ones(hidden_size, dtype=np.float32)
        tensors[f"{prefix}.post_attention_layernorm.weight"] = np.ones(hidden_size, dtype=np.float32)
    
    # Final norm
    tensors["model.norm.weight"] = np.ones(hidden_size, dtype=np.float32)
    
    # Save
    output_path.parent.mkdir(parents=True, exist_ok=True)
    save_file(tensors, str(output_path))
    
    total_size = sum(v.nbytes for v in tensors.values())
    print(f"[OK] Generated test weights: {output_path}")
    print(f"[OK] Total size: {total_size / 1024 / 1024:.1f} MB")
    print(f"[OK] Number of tensors: {len(tensors)}")


def main():
    parser = argparse.ArgumentParser(description="Generate test weights for Qwen2")
    parser.add_argument("--config", type=Path, default="models/dev-qwen2-fixture/config.json",
                       help="Path to config.json")
    parser.add_argument("--output", type=Path, default="models/Qwen2.5-0.5B/model.safetensors",
                       help="Output path for weights")
    args = parser.parse_args()
    
    generate_weights(args.config, args.output)


if __name__ == "__main__":
    main()
