#!/usr/bin/env python3

# Copyright Amlogic

import argparse
import logging
import os
import sys
import random

from Cryptodome.PublicKey import ECC
from Cryptodome.Cipher import AES

def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description='\n\
    Encrypts OTP secret for programming.\n\
    encrypt_secret.py -k vendor_priv.pem -d device_pub.pem -i dgpk1.bin -o edgpk1.bin -x edpgk1.txt')

    parser.add_argument('-k', '--private-key',
                        metavar='vendor_priv.pem',
                        required=True,
                        help='Input vendor private key file',
                        )
    parser.add_argument('-d', '--device-public-key',
                        metavar='device_public_key',
                        required=True,
                        help='Input device public key in hex string',
                        )
    parser.add_argument('-i', '--input',
                        metavar='input',
                        required=True,
                        help='Input secret file',
                        )
    parser.add_argument('-o', '--output',
                        metavar='output',
                        required=False,
                        help='Output encrypted secret file',
                        )
    parser.add_argument('-x', '--output-hex',
                        metavar='output',
                        required=False,
                        help='Output encrypted secret file in hex',
                        )

    parser.add_argument('-v', '--verbose',
                        action='store_true',
                        help='Enable verbose log')

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)

    logging.info('Running {}'.format(sys.argv))

    return args

def key_import(key):
    """ Check keys are valid, on the curve.
        returns status, EccKey
    """
    if not key:
        logging.error('key arg missing')
        return False, None
    if len(key) < 128:
        logging.error('key arg should be 64 bytes in hex')
        logging.error(' {}'.format(len(key)))
        return False, None
    kb = bytes.fromhex(key)
    if len(kb) != 64:
        logging.error('key arg should be 64 bytes in hex')
        logging.error(' {}'.format(len(kb)))
        return False, None
    xb = kb[:32]
    yb = kb[32:]
    x = int.from_bytes(xb, byteorder='big', signed=False)
    y = int.from_bytes(yb, byteorder='big', signed=False)
    ecc_key = ECC.construct(curve='p256', point_x=x, point_y=y)

    return True, ecc_key

if __name__ == '__main__':
    args = parse_args()

    if not os.path.exists(args.private_key):
        logging.error('Private key {} does not exist'.format(args.private_key))
        sys.exit(1)
    device_key_r, device_key = key_import(args.device_public_key)
    if not device_key_r:
        logging.error('Device public key is invalid')
        sys.exit(1)
    if not os.path.exists(args.input):
        logging.error('Input {} does not exist'.format(args.input))
        sys.exit(1)
    if not args.output and not args.output_hex:
        logging.error('Missing output arg')
        sys.exit(1)
    if os.path.exists(args.output):
        logging.error('Output {} already exists'.format(args.output))
        sys.exit(1)
    if os.path.exists(args.output_hex):
        logging.error('Output {} already exists'.format(args.output_hex))
        sys.exit(1)

    # Load keys
    with open(args.private_key, 'rt') as f:
        vendor_key = ECC.import_key(f.read())

    # Derive shared secret with ECDH
    pointZ = device_key.pointQ * vendor_key.d
    shared_secret = int.to_bytes(int(pointZ.x), length=32, byteorder='big')

    # AES-GCM encrypt
    # randbytes only in 3.9+
    iv = bytes([random.randint(0, 255) for _ in range(12)])

    cipher = AES.new(shared_secret, AES.MODE_GCM, nonce=iv, mac_len=16)
    with open(args.input, 'rb') as f:
        pt = f.read()
        ct, tag = cipher.encrypt_and_digest(pt)

    with open(args.output, 'wb') as f:
        f.write(ct)
        f.write(iv)
        f.write(tag)
    with open(args.output_hex, 'wt') as f:
        f.write(ct.hex())
        f.write(iv.hex())
        f.write(tag.hex())
        f.write("\n")

    #logging.debug(f'out(enc_efuseinfo)= {ct.hex()}{iv.hex()}{tag.hex()}')

    sys.exit(0)
