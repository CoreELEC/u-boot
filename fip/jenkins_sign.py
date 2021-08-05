#!/usr/bin/env python3

import sys
import os
import re
import requests
from requests.auth import HTTPBasicAuth
from config import *
from urllib.parse import urljoin
import urllib.request
import json
import time

auth = HTTPBasicAuth(user, password)

def get_args():
    from argparse import ArgumentParser

    parser = ArgumentParser()
   
    parser.add_argument('--in', dest='inputFilePath', required=True,
			help='input file')
    parser.add_argument('--chipAcsFile', dest='chipAcsFilePath',
                        default='null', help='chip acs file')
    parser.add_argument('--out', dest = 'outFilePath', type=str,
			default='', help='output signed file')
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 1.0', help='version')
    parser.add_argument('--type', choices=types,
            default=types[0], required = True )
    parser.add_argument('--chip', type=str)
    parser.add_argument('--taVersion', type = int,
            default=0)
    parser.add_argument('--casProvider', choices=casProviders, 
            default=casProviders[0])
    parser.add_argument('--ddrType', choices=ddrTypes,
            default=ddrTypes[0])
    parser.add_argument('--chipVariant', choices=chipVariants,
            default=chipVariants[0])
    parser.add_argument('--keyType', type=str, dest='keyType', default='dev-keys')
   
    return parser.parse_args()

def getLastBuildNumber(rootJobUrl):
    url = urljoin(rootJobUrl, "lastBuild/buildNumber")

    response = requests.get(url, auth=auth)

    if response.status_code == 200:
        return response.text
    else:
        print("Fail to get last build number due to the error " + str(response.status_code))
        return 0

def getJobRootUrl(type):
    if(type == 'ta'):
        return urljoin(serverRootUrl, "Sign_TA/")
    elif(type == 'bl31'):
        return urljoin(serverRootUrl, "Sign_Bl31/")
    elif(type == 'bl2'):
        return urljoin(serverRootUrl, "Sign_Bl2/")
    elif(type == 'bl32'):
        return urljoin(serverRootUrl, "Sign_Bl32/")
    elif(type == 'fw'):
        return urljoin(serverRootUrl, "Sign_FW/")
    else:  # bl2e, bl2x, bl40
        return urljoin(serverRootUrl, "Sign_Bl2e_Bl2x_Bl40/")

def submitSignJob(type, chipType, inputFilePath, chipAcsFilePath, taVersion="0", casProvider="", chipVariant="", ddrType="", keyType="dev-keys"):

    fileName = os.path.basename(inputFilePath)
    fileParameter = 'file'
    uploadFile = {
        fileParameter: (fileName, open(inputFilePath, 'rb')),

    }
    url = getJobRootUrl(type) + "buildWithParameters"
    if(type == 'ta'):
        data = {
            'chip_part_number': chipType,
            'ta_version': taVersion,
        }
    elif(type == 'bl32'):

        data = {
            'chipPartNumber': chipType,
            'casProvider': casProvider,
            'keyType': keyType
        }

    elif(type == 'bl2'):
        chipAcsfileName = os.path.basename(chipAcsFilePath)
        uploadFile = {
                fileParameter: (fileName, open(inputFilePath, 'rb')),
                "chipAcsFile": (chipAcsfileName, open(chipAcsFilePath, 'rb'))
        }
        data = {
            'chipPartNumber': chipType,
            'chipVariant': chipVariant,
            'ddrType': ddrType,
            'keyType': keyType
        }
    else:  # bl2e, bl2x, bl40,
        data = {
            'chipPartNumber': chipType,
            'keyType': keyType
        }

    response = requests.post(url, auth=auth,
                             data=data,
                             files=uploadFile)

    if response.status_code == 201:
        print("Sumbit signing job successfully, please wait...")
      
    else:
        print("Fail to start signing job due to the error: " + str(response.status_code) ) 
        exit(1)
        
def queryBuildStatus(rootJobUrl, buildNumber):
    url = rootJobUrl + str(buildNumber) + "/api/json?tree=building"

    response = requests.get(url, auth=auth)

    if response.status_code == 200:
        result = json.loads(response.text)
        return str(result['building'])
    else:
        return "NotStart"

def downloadSignedFile(rootJobUrl, buildNumber, inFileDir="",specifiedOutFilePath=""):

    url = rootJobUrl + str(buildNumber) + \
        "/api/json?tree=artifacts[relativePath]"

    response = requests.get(url, auth=auth)

    if response.status_code == 200:
        result = json.loads(response.text)
        if(len(result['artifacts']) == 0):
            print("Fail to build, please check jenkins log for detailed error")
            exit(1)
        relativePath = result['artifacts'][0]['relativePath']
        #http://127.0.0.1:8080/job/Sign_Bl31/46/artifact/46/output/bl31-payload.bin.signed
        downloadUrl = rootJobUrl + str(buildNumber) + "/artifact/"  + "/" + relativePath
        if(specifiedOutFilePath == ""):
           outFilePath = os.path.join(inFileDir,os.path.basename(relativePath) )
        else: 
           outFilePath = specifiedOutFilePath
        r = requests.get(downloadUrl, auth=auth)
        with open(outFilePath, 'wb') as f:
            f.write(r.content)
        print("Download the signed file at " + outFilePath)    
        return 0
    else:
        print("Fail to download the signed file")
        exit(1)
        return 1
        
def main():
    args = get_args()
    
    rootJobUrl = getJobRootUrl(args.type)
    lastBuildNumber = getLastBuildNumber(rootJobUrl)

    submitSignJob(
        type = args.type, 
        chipType = args.chip,
        inputFilePath = args.inputFilePath, 
        chipAcsFilePath = args.chipAcsFilePath,
        taVersion = args.taVersion,
        casProvider = args.casProvider, 
        chipVariant = args.chipVariant,
        ddrType = args.ddrType,
        keyType = args.keyType
        )
    buildNumber = int(lastBuildNumber) + 1
    print("The jenkins build number: " + str(buildNumber))
    while True:
        time.sleep(1)
        building = queryBuildStatus(rootJobUrl, buildNumber)
        print("Building Status= " + str(building))
        if building == "False":
            print("Build is done. Will start to download the signed file")
            break
    inputFileDir = os.path.dirname(args.inputFilePath)    
    downloadSignedFile(rootJobUrl, buildNumber, inputFileDir,args.outFilePath)
   
if __name__ == "__main__":
    main()
