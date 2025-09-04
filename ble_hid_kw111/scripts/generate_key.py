import secrets
import binascii

# Genera 32 byte casuali (256 bit)
key = secrets.token_bytes(32)
print("Chiave generata:", binascii.hexlify(key).decode())

# Salva in un file binario
with open("hmac_key.bin", "wb") as f:
    f.write(key)

# # Scrivi la chiave nell'eFuse HMAC_KEY0
# espefuse.py --port COM9 burn_key BLOCK_KEY0 hmac_key.bin HMAC_UP

# # Verifica che sia stata scritta correttamente
# espefuse.py --port COM9 summary