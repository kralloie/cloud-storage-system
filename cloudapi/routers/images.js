const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
let { tokens } = require('../token.js')
const { insertLog } = require('../tools/loghandler.js');

const imagesRouter = express.Router();
const storageDir = path.join(path.dirname(__filename),'../storage/')
const upload = multer();

imagesRouter.get('/',(req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    const file = req.query.file;
    const username = req.query.username
    const fileDir = path.join(storageDir,`${username}/images/${file}`);
    const fileExt = path.extname(file);
    fs.readdir(path.join(storageDir,`${username}/images/`), (err,files) =>{
        if(files.includes(file)){
            fs.readFile(fileDir,(err,data) =>{
                if(err)
                {
                    throw err
                }
                insertLog(username,token,file,"download");
                return res.status(200)
                .contentType(mime.lookup(fileExt))
                .setHeader('File-Type','image')
                .send(data);
            });
        } 
        else 
        { 
            return res.status(404)
            .send('File not found \n'); 
        }
    })
})

imagesRouter.get('/admin',(req,res) =>{
    const token = req.query.api;
    const adminName = req.query.admin;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    const file = req.query.file;
    const username = req.query.username
    const fileDir = path.join(storageDir,`${username}/images/${file}`);
    const fileExt = path.extname(file);
    fs.readdir(path.join(storageDir,`${username}/images/`), (err,files) =>{
        if(files.includes(file)){
            fs.readFile(fileDir,(err,data) =>{
                if(err)
                {
                    throw err
                }
                insertLog(adminName,token,file,"download");
                return res.status(200)
                .contentType(mime.lookup(fileExt))
                .setHeader('File-Type','image')
                .send(data);
            });
        } 
        else 
        { 
            return res.status(404)
            .send('File not found \n'); 
        }
    })
})

imagesRouter.post('/', upload.single('file'),(req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    console.log('Post request received');
    var fileName = req.query.file;
    const username = req.query.username;
    const [fName,fExt] = fileName.split('.');
    const fileBuffer = req.file.buffer;
    var duplicatedNum = 0;
    fs.readdir(path.join(storageDir,`${username}/images/`),(err,files) =>{
        while(files.includes(fileName))
        {
            duplicatedNum++;
            fileName = `${fName}(${duplicatedNum}).${fExt}`;
        }

        const saveDir = path.join(storageDir,`${username}/images/${fileName}`);
        fs.writeFile(saveDir, fileBuffer, (err) =>{
            if(err)
            {
               return res.status(500)
               .send('Error while saving the file. \n');
            }
            insertLog(username,token,fileName,"upload")
            return res.status(200)
            .send('File saved correctly \n');
        })
    })
})

imagesRouter.delete('/',(req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    const file = req.query.file;
    const username = req.query.username;
    fs.readdir(path.join(storageDir,`${username}/images`), (err,files) =>{              
        fs.unlink(path.join(storageDir,`${username}/images/${file}`), err =>{
            if(err)
            {
                console.log('couldnt delete');
                return res.status(500)
                .send('Error while deleting the file. \n')
            }
            insertLog(username,token,file,"delete")
            res.status(200)
            .send(`File deleted successfully \n`)
        })
    })
})

module.exports.imagesRouter = imagesRouter;

