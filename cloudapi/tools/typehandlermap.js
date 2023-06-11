const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const bodyParser = require('body-parser');
const {PDFDocument, StandardFonts} = require('pdf-lib');

class typeMap extends Map
{
    constructor(entries)
    {
        super(entries);
    }

    get(key)
    {
        if(this.has(key))
        {
            return super.get(key);
        }
        else
        {
            return super.get('fallback');
        }
    }
}

const typemap = new typeMap([
    ['application/json', fileBuffer =>{
        console.log("called");
        const fileJSON = JSON.parse(fileBuffer);
        const JSONString = JSON.stringify(fileJSON,null,4); 
        return new Promise( resolve =>{
            resolve(JSONString);
        })
    }],
    ['text/plain',fileBuffer =>{
        const fileString = fileBuffer.toString();
        return new Promise(resolve =>{
            resolve(fileString);
        })
    }],
    ['application/pdf',async fileBuffer =>{
        const fileString = fileBuffer.toString()
        return new Promise(resolve => {
            resolve(fileString);
        })
    }],
    ['fallback', async fileBuffer =>{
        const fileString = fileBuffer.toString()
        return new Promise(resolve =>{
            resolve(fileString);
        })
    }]
])

module.exports.typemap = typemap;