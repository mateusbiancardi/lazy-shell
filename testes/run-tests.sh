#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
inputs_dir="$root_dir/testes/input"
outputs_dir="$root_dir/testes/output"
results_dir="$root_dir/testes/result"
shell_bin="$root_dir/main"

mkdir -p "$results_dir"

pass=0
fail=0

normalize_prompted_output() {
  sed -E 's/^(lsh> )+//' "$1" | sed '/^$/d' | sort
}

gen_result() {
  local input_file="$1"
  local base
  base="$(basename "$input_file")"
  local out_file="$results_dir/$base"
  local exp_file="$outputs_dir/$base"
  local fifo

  fifo="$(mktemp -u)"
  mkfifo "$fifo"

  "$shell_bin" < "$fifo" > "$out_file" &
  local pid=$!

  exec 3> "$fifo"
  sleep 0.2
  while IFS= read -r line || [ -n "$line" ]; do
    if [ "$line" = "^C" ]; then
      kill -INT "$pid" 2>/dev/null || true
      sleep 0.1
    else
      printf '%s\n' "$line" >&3
      sleep 0.05
    fi
  done < "$input_file"
  exec 3>&-

  rm -f "$fifo"
  for _ in {1..30}; do
    if ! kill -0 "$pid" 2>/dev/null; then
      break
    fi
    sleep 0.1
  done
  if kill -0 "$pid" 2>/dev/null; then
    kill -TERM "$pid" 2>/dev/null || true
    sleep 0.1
  fi
  wait "$pid" 2>/dev/null || true

  if [ -f "$exp_file" ]; then
    if [[ "$base" == "caso1.txt" || "$base" == "caso13.txt" ]]; then
      if diff -u <(normalize_prompted_output "$exp_file") \
        <(normalize_prompted_output "$out_file") >/dev/null; then
        echo "[ok] $base"
        pass=$((pass + 1))
      else
        echo "[fail] $base"
        fail=$((fail + 1))
      fi
    elif diff -u "$exp_file" "$out_file" >/dev/null; then
      echo "[ok] $base"
      pass=$((pass + 1))
    else
      echo "[fail] $base"
      fail=$((fail + 1))
    fi
  else
    echo "[skip] $base (sem saida esperada)"
  fi
}

for input_file in "$inputs_dir"/caso*.txt; do
  gen_result "$input_file"
done

echo ""
echo "passou $pass falhou $fail"
