const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const bodyParser = require('body-parser');
const {PDFDocument, StandardFonts} = require('pdf-lib');

const typemap = new Map([
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
        var pdfDoc;
        PDFDocument.load(fileBuffer)
        .then((result) =>{
            pdfDoc = result;
        })
        const pages = pdfDoc.getPages();
    }]
])

module.exports.typemap = typemap;