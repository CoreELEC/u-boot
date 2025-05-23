#!/usr/bin/env python3

# Copyright Amlogic

import argparse
import logging
import os
import sys

from Cryptodome.PublicKey import ECC

def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description='\n\
    Generates partner an ECC keypair.\n\
    gen_keypair.py -k PRIV_KEY -u PUB_KEY')

    parser.add_argument('-k', '--private-key',
                        metavar='vendor_priv.pem',
                        required=True,
                        help='Output private key file',
                        )
    parser.add_argument('-u', '--public-key',
                        metavar='vendor_pub.pem',
                        required=True,
                        help='output public key file',
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

if __name__ == '__main__':
    args = parse_args()

    logging.debug(f'{args.private_key=}')
    logging.debug(f'{args.public_key=}')

    if os.path.exists(args.private_key):
        logging.error('Private key {} already exists'.format(args.private_key))
        sys.exit(1)
    if os.path.exists(args.public_key):
        logging.error('Public key {} already exists'.format(args.public_key))
        sys.exit(1)

    key = ECC.generate(curve='p256')


    logging.info('Writing private key to {}'.format(args.private_key))
    with open(args.private_key, 'wt') as f:
        f.write(key.export_key(format='PEM'))

    logging.info('Writing public key to {}'.format(args.public_key))
    with open(args.public_key, 'wt') as f:
        f.write(key.public_key().export_key(format='PEM'))

    sys.exit(0)
