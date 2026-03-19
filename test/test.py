# Quelques tests avec l'aes éprouvé de SCAred

from scared import aes
import numpy as np


# Convertir en bytes d'abord
key_int = 0xe3649b41fc56949d6bdfb7a49cfdd624
key_bytes = key_int.to_bytes(16, byteorder='big')
plain = 0x00000000000000000000000000000000
plain_bytes = plain.to_bytes(16, byteorder='big')

# Convertir en numpy array
key_array = np.frombuffer(key_bytes, dtype=np.uint8)
result = aes.inv_key_schedule(key_array, 4)

master_key = result[0, 0]
hex_string = master_key.tobytes().hex()
plaintext = np.frombuffer(plain_bytes, dtype=np.uint8)

print(plaintext)
print(hex_string) 
print(result[0, 4].tobytes().hex())

ciphertext = aes.encrypt(plaintext, master_key, at_round=4)
print(ciphertext.tobytes().hex())



print("Initial plaintext:")
print(plaintext.tobytes().hex())

# Afficher le State après chaque étape jusqu'au round 4
print("\nAfter AddRoundKey[0]:")
result = aes.encrypt(plaintext, master_key, at_round=0, after_step=aes.Steps.ADD_ROUND_KEY)
print(result.tobytes().hex())

for r in range(1, 4):
    print(f"\nRound {r}:")
    
    print(f"  After SubBytes:")
    result = aes.encrypt(plaintext, master_key, at_round=r, after_step=aes.Steps.SUB_BYTES)
    print(f"    {result.tobytes().hex()}")
    
    print(f"  After ShiftRows:")
    result = aes.encrypt(plaintext, master_key, at_round=r, after_step=aes.Steps.SHIFT_ROWS)
    print(f"    {result.tobytes().hex()}")
    
    print(f"  After MixColumns:")
    result = aes.encrypt(plaintext, master_key, at_round=r, after_step=aes.Steps.MIX_COLUMNS)
    print(f"    {result.tobytes().hex()}")
    
    print(f"  After AddRoundKey:")
    result = aes.encrypt(plaintext, master_key, at_round=r, after_step=aes.Steps.ADD_ROUND_KEY)
    print(f"    {result.tobytes().hex()}")

print(f"\nRound 4 (last round):")

print(f"  After SubBytes:")
result = aes.encrypt(plaintext, master_key, at_round=4, after_step=aes.Steps.SUB_BYTES)
print(f"    {result.tobytes().hex()}")

print(f"  After ShiftRows:")
result = aes.encrypt(plaintext, master_key, at_round=4, after_step=aes.Steps.SHIFT_ROWS)
print(f"    {result.tobytes().hex()}")

print(f"  After AddRoundKey:")
result = aes.encrypt(plaintext, master_key, at_round=4, after_step=aes.Steps.ADD_ROUND_KEY)
print(f"    {result.tobytes().hex()}")


print(f"\nFinal (Round 4):")
result = aes.encrypt(plaintext, master_key, at_round=4, after_step=aes.Steps.ADD_ROUND_KEY)
print(result.tobytes().hex())


