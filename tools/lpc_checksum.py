#!/usr/bin/env python3
"""Écrit la somme de contrôle LPC17xx dans le mot 7 de l'image binaire.
La ROM de boot n'exécute le code que si la somme des 8 premiers mots vaut 0."""
import struct, sys

path = sys.argv[1]
data = bytearray(open(path, "rb").read())
words = list(struct.unpack_from("<8I", data, 0))
words[7] = (-sum(words[:7])) & 0xFFFFFFFF
struct.pack_into("<I", data, 28, words[7])
open(path, "wb").write(data)
print(f"checksum 0x{words[7]:08X} -> {path}")
