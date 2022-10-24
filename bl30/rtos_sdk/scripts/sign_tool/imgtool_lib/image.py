# -*- coding: utf-8 -*-
# Copyright 2017 Linaro Limited
# Copyright (c) 2018-2019, Arm Limited.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Image signing and management.
"""

from . import version as versmod
from ctypes import create_string_buffer
import binascii
import hashlib
import struct, os
from imgtool_lib import keys

IMAGE_MAGIC = 0x96F3B83D
IMAGE_MAGIC_BL2 = 0x42544844
IMAGE_HEADER_SIZE = 32
TLV_HEADER_SIZE = 4
PAYLOAD_DIGEST_SIZE = 32  # SHA256 hash
KEYHASH_SIZE = 32

# Image header flags.
IMAGE_F = {
    'PIC': 0x0000001,
    'NON_BOOTABLE': 0x0000010,
    'RAM_LOAD': 0x0000020, }
TLV_VALUES = {
    'KEYHASH': 0x01,
    'SHA256': 0x10,
    'RSA2048': 0x20,
    'RSA3072': 0x23,
    'RSA4096': 0x71,#?
    'SEC_CNT': 0x50, }

TLV_INFO_SIZE = 4
TLV_INFO_MAGIC = 0x6907

# Sizes of the image trailer, depending on flash write size.
trailer_sizes = {
    write_size: 128 * 3 * write_size + 8 * 2 + 16
    for write_size in [1, 2, 4, 8]
}

boot_magic = bytearray([
    0x77, 0xc2, 0x95, 0xf3,
    0x60, 0xd2, 0xef, 0x7f,
    0x35, 0x52, 0x50, 0x0f,
    0x2c, 0xb6, 0x79, 0x80, ])


class TLV:
    def __init__(self):
        self.buf = bytearray()

    def add(self, kind, payload):
        """Add a TLV record.  Kind should be a string found in TLV_VALUES above."""
        buf = struct.pack('<BBH', TLV_VALUES[kind], 0, len(payload))
        self.buf += buf
        self.buf += payload

    def get(self):
        header = struct.pack('<HH', TLV_INFO_MAGIC, TLV_INFO_SIZE + len(self.buf))
        return header + bytes(self.buf)


class Image():
    @classmethod
    def load(cls, path, included_header=False, **kwargs):
        """Load an image from a given file"""
        with open(path, 'rb') as f:
            payload = f.read()
        obj = cls(**kwargs)
        obj.payload = payload

        # Add the image header if needed.
        # print("Payload(initial) len = " + str(len(payload)))
        if not included_header and obj.header_size > 0:
            obj.payload = (b'\000' * obj.header_size) + obj.payload
        # print("Payload(add-header) len = " + str(len(obj.payload)))
        obj.check()
        return obj

    def __init__(self, version=0, header_size=IMAGE_HEADER_SIZE, security_cnt=0,
                 pad=0):
        self.version = version
        self.header_size = header_size or IMAGE_HEADER_SIZE
        self.security_cnt = security_cnt
        self.pad = pad

    def __repr__(self):
        return "<Image version={}, header_size={}, security_counter={}, \
                 pad={}, payloadlen=0x{:x}>".format(
            self.version,
            self.header_size,
            self.security_cnt,
            self.pad,
            len(self.payload))

    def save(self, path):
        with open(path, 'wb') as f:
            f.write(self.payload)

    def check(self):
        """Perform some sanity checking of the image."""
        # If there is a header requested, make sure that the image
        # starts with all zeros.
        if self.header_size > 0:
            if any(v != 0 and v != b'\000' for v in self.payload[0:self.header_size]):
                raise Exception("Padding requested, but image does not start with zeros")
        # pad 16
        pad_size = 16 - int(len(self.payload) % 16)
        if 16 > pad_size > 0:
            self.payload += (b'\000' * pad_size)
        # print("Payload(pad-16) len = " + str(len(self.payload)))

    def sign(self, key, ramLoadAddress):
        # Size of the security counter TLV:
        # header ('BBH') + payload ('I') = 8 Bytes
        protected_tlv_size = TLV_INFO_SIZE + 8  # = 12 Byte

        self.add_header(key, protected_tlv_size, ramLoadAddress)

        tlv = TLV()
        #print("Payload len (add_header) = " + str(len(self.payload)))
        payload = struct.pack('I', self.security_cnt)
        tlv.add('SEC_CNT', payload)

        # Full TLV size needs to be calculated in advance, because the
        # header will be protected as well
        # 4 + 4 + 4 + 32 = 44 Byte
        full_size = (TLV_INFO_SIZE + len(tlv.buf) + TLV_HEADER_SIZE
                     + PAYLOAD_DIGEST_SIZE)

        if key is not None:
            full_size += (TLV_HEADER_SIZE + KEYHASH_SIZE
                          + TLV_HEADER_SIZE + key.sig_len())
        # 4 Byte
        tlv_header = struct.pack('HH', TLV_INFO_MAGIC, full_size)
        self.payload += tlv_header + bytes(tlv.buf)  # 4 + 8

        sha = hashlib.sha256()
        sha.update(self.payload)
        digest = sha.digest()
        tlv.add('SHA256', digest)

        if key is not None:
            pub = key.get_public_bytes()
            sha = hashlib.sha256()
            sha.update(pub)
            pubbytes = sha.digest()
            tlv.add('KEYHASH', pubbytes)

            sig = key.sign(self.payload)
            tlv.add(key.sig_tlv(), sig)

        self.payload += tlv.get()[protected_tlv_size:]
        #print("Payload len (add_ender) = " + str(len(self.payload)))

    def sign_for_bl2(self, args):
        ## set sign image header
        sign_image_header = Sign_Image_Header()
        sign_image_header.payload_size = len(self.payload[self.header_size:])
        sign_image_header.payload_offset = 512
        sign_image_header.cert_offset = len(self.payload) + 96
        # print("sign_image_header.payload_size = %d" % sign_image_header.payload_size)
        payload_hash = self.cal_sha256(self.payload[self.header_size:])
        # print(payload_hash)
        ## add header
        self.add_header_for_bl2(payload_hash)
        ## get key
        temp_path = args.path_of_key
        if not temp_path:
            raise Exception("args.path_of_key error!")
        #  ====RSA=====
        if args.key_type:
            pri_key = None
            pub_key = None
            for temp_file_name in os.listdir(temp_path):
                if "private" in temp_file_name:
                    pri_key = keys.load_rsa(os.path.join(temp_path, temp_file_name))
                if "public" in temp_file_name:
                    pub_key = keys.load_rsa(os.path.join(temp_path, temp_file_name))
            if not pri_key:
                raise Exception("bl2 pri_key error!")
            if not pub_key:
                raise Exception("bl2 pub_key error!")
            ## set sign_key_cert
            sign_key_cert = Sign_Rsa_Key_Cert()
            sign_key_cert.certtype = 1
            if args.key is not None:
                if not os.path.exists(args.key):
                    raise Exception("args.key not exists!")
                rtos_pud_key = keys.load_rsa(args.key)
                sign_key_cert.pub_key = rtos_pud_key.get_public_bytes_for_rsa() + b"\000" * (512-rtos_pud_key.sig_len())#pud_key.get_public_bytes() + b"\000" * (512-pud_key.sig_len())
            else:
                sign_key_cert.pub_key = b"\000" * 512
            sign_key_cert.version = 0
            sign_key_cert.image_size = len(self.payload[self.header_size:])
            sign_key_cert.kce_enable = 0
            sign_key_cert.type = 1
            sign_key_cert.cbc_iv = b'\000' * 16
            sign_key_cert.hash_data = payload_hash
            sign_key_cert.hash_key = self.cal_sha256(pub_key.get_public_bytes_for_rsa() + b"\000" * (512-pub_key.sig_len()))#pub_key.get_public_bytes())
            sign_key_cert.signature = pri_key.sign(sign_key_cert.hash_data + sign_key_cert.hash_key +
                                                   struct.pack("i",sign_key_cert.type) + struct.pack("i",sign_key_cert.version))
            sign_key_cert.signature += b"\000" *(512-len(sign_key_cert.signature))
            # self.print_hex(sign_key_cert.signature)
            temp_key_cert = sign_key_cert.get_sign_key_cert()
            # print(len(temp_key_cert))
            # print(temp_key_cert)
            sign_image_header.cert_size = len(temp_key_cert)
            sign_image_header.cert_dbg_prim_offset = sign_image_header.cert_offset + sign_image_header.cert_size
            sign_image_header.cert_dbg_prim_size = 1
            sign_image_header.cert_dbg_developer_offset = sign_image_header.cert_offset + sign_image_header.cert_size
            sign_image_header.cert_dbg_developer_size = 1
            temp_sign_header = sign_image_header.get_sign_image_header()
            # print(len(temp_sign_header))
            # self.print_hex(temp_sign_header)
            self.payload += temp_sign_header
            self.payload += temp_key_cert
    def cal_sha256(self, data):
        sha = hashlib.sha256()
        sha.update(data)
        return sha.digest()

    def add_header(self, key, protected_tlv_size, ramLoadAddress):
        """Install the image header.

        The key is needed to know the type of signature, and
        approximate the size of the signature."""

        flags = 0
        if ramLoadAddress is not None:
            # add the load address flag to the header to indicate that an SRAM
            # load address macro has been defined
            flags |= IMAGE_F["RAM_LOAD"]

        fmt = ('<' +
               # type ImageHdr struct {
               'I' +  # Magic    uint32
               'I' +  # LoadAddr uint32
               'H' +  # HdrSz    uint16
               'H' +  # PTLVSz   uint16
               'I' +  # ImgSz    uint32
               'I' +  # Flags    uint32
               'BBHI' +  # Vers     ImageVersion
               'I'  # Pad1     uint32
               )  # }
        assert struct.calcsize(fmt) == IMAGE_HEADER_SIZE
        header = struct.pack(fmt,
                             IMAGE_MAGIC,
                             0 if (ramLoadAddress is None) else ramLoadAddress,  # LoadAddr
                             self.header_size,
                             protected_tlv_size,  # TLV info header + security counter TLV
                             len(self.payload) - self.header_size,  # ImageSz
                             flags,  # Flags
                             self.version.major or 0,
                             self.version.minor or 0,
                             self.version.revision or 0,
                             self.version.build or 0,
                             0)  # Pad1
        self.payload = bytearray(self.payload)
        #print("add_header:" + self.print_hex(header))
        self.payload[:len(header)] = header

    def add_header_for_bl2(self, hash):
        """Install the image header."""
        # assert struct.calcsize(fmt) == IMAGE_HEADER_SIZE
        header = struct.pack("I", IMAGE_MAGIC_BL2)
        header += struct.pack("I", 1)  # MagicV
        header += hash  # mPayloadHash[32]
        header += struct.pack("I", 0)  # mImgAddr
        header += struct.pack("I", len(self.payload) - self.header_size)  # mImgSize
        self.payload = bytearray(self.payload)
        self.payload[:len(header)] = header

    def pad_to(self, size, align):
        """Pad the image to the given size, with the given flash alignment."""
        tsize = trailer_sizes[align]
        padding = size - (len(self.payload) + tsize)
        if padding < 0:
            msg = "Image size (0x{:x}) + trailer (0x{:x}) exceeds requested size 0x{:x}".format(
                len(self.payload), tsize, size)
            raise Exception(msg)
        pbytes = b'\xff' * padding
        pbytes += b'\xff' * (tsize - len(boot_magic))
        pbytes += boot_magic
        self.payload += pbytes

    def print_hex(self, temp_bytes):
        # print(temp_bytes)
        l = [hex(int(i)) for i in temp_bytes]
        temp_data = ""
        for count, b in enumerate(l):
            if count % 16 == 0:
                print(temp_data)
                temp_data = ""
            else:
                temp_data += b + " "
        print(temp_data)


class Sign_Image_Header:
    def __init__(self):
        ##Magic number 8 Byte
        self.magic = 0x6161616161616161  # aaaaaaaa
        ##Version of this header format 4 + 4 Byte
        self.header_version_major = 0  # 4 Byte
        self.header_version_minor = 0  # 4 Byte
        ##image body, plain or cipher text
        self.payload_size = 0  # 8 Byte
        self.payload_offset = 0  # 8 Byte
        ##offset from itself start
        ##content certification size, if 0, ignore
        self.cert_size = 0  # 8 Byte
        self.cert_offset = 0  # 8 Byte
        ##(opt)private content size,if 0,ignore
        self.priv_size = 0  # 8 Byte
        self.priv_offset = 0  # 8 Byte
        ##(opt)debug/rma certification primary size,if 0,ignore
        self.cert_dbg_prim_size = 0  # 8 Byte
        self.cert_dbg_prim_offset = 0  # 8 Byte
        ##(opt)debug/rma certification second size,if 0,ignore
        self.cert_dbg_developer_size = 0  # 8 Byte
        self.cert_dbg_developer_offset = 0  # 8 Byte

    def get_sign_image_header(self):
        sign_image_header = bytearray()
        sign_image_header += struct.pack("<qiiqqqqqqqqqq",
                                         self.magic,
                                         self.header_version_major,
                                         self.header_version_minor,
                                         self.payload_size,
                                         self.payload_offset,
                                         self.cert_size,
                                         self.cert_offset,
                                         self.priv_size,
                                         self.priv_offset,
                                         self.cert_dbg_prim_size,
                                         self.cert_dbg_prim_offset,
                                         self.cert_dbg_developer_size,
                                         self.cert_dbg_developer_offset)
        return sign_image_header


class Sign_Rsa_Key_Cert:
    def __init__(self):
        self.certtype = 1  # 1:key cert
        ###sprd_rsapubkey 4 + 4 + 512 = 520 Byte
        # self.keybit_len = 0 #//1024/2048,max 2048
        # self.e = 0
        # self.mod = b'' # 512 Byte
        self.pub_key = b""
        ################
        self.hash_data = b""  # hash_data[HASH_BYTE_LEN]    hash of current image data  32 Byte
        self.hash_key = b""  # hash_key[HASH_BYTE_LEN]	  hash of pubkey in next cert 32 Byte
        self.type = 0
        self.version = 0
        self.image_size = 0
        self.kce_enable = 0
        self.cbc_iv = b""  # 16 Byte
        self.signature = b""  # RSA_KEY_BYTE_LEN_MAX = 512 Byte

    def get_sign_key_cert(self):
        temp_sign_key_cert = bytearray()
        temp_sign_key_cert += struct.pack("I", self.certtype)
        temp_sign_key_cert += self.pub_key
        temp_sign_key_cert += self.hash_data
        temp_sign_key_cert += self.hash_key
        temp_sign_key_cert += struct.pack("I", self.type)
        temp_sign_key_cert += struct.pack("I", self.version)
        temp_sign_key_cert += struct.pack("I", self.image_size)
        temp_sign_key_cert += struct.pack("I", self.kce_enable)
        temp_sign_key_cert += self.cbc_iv
        temp_sign_key_cert += self.signature
        return temp_sign_key_cert


class RSA_PUB_KEY:
    def __init__(self):
        pass

    def get_ras_pub_key(self):
        temp_rsa_pub_key = bytearray()
        return temp_rsa_pub_key
