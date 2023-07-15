const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const { typemap } = require('../tools/typehandlermap');
let { tokens } = require('../token.js')
const { insertLog } = require('../tools/loghandler');

const textRouter = express.Router();
const storageDir = path.join(path.dirname(__filename),'../storage/')
const upload = multer();

textRouter.get('/',(req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    const file = req.query.file;
    const username = req.query.username;
    const fileExt = path.extname(file);

    if(fileExt == "")
    {
        var fileType = ".txt"
    }
    else
    {
        fileType = fileExt;
    }

    const fileDir = path.join(storageDir,`${username}/texts/${file}`);
    fs.readdir(path.join(storageDir,`${username}/texts/`), (err,files) =>{
        if(files.includes(file)){
            fs.readFile(fileDir,(err,data) =>{
                if(err)
                {
                    throw err
                }
                insertLog(username,token,file,"download");
                return res.status(200)
                .contentType(mime.lookup(fileType))
                .setHeader('File-Type','text')
                .send(data);
            });
        } else { return res.status(404).send("File not found"); }
    })
})

textRouter.get('/admin',(req,res) =>{
    const token = req.query.api;
    const adminName = req.query.admin;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    const file = req.query.file;
    const username = req.query.username;
    const fileExt = path.extname(file);

    if(fileExt == "")
    {
        var fileType = ".txt"
    }
    else
    {
        fileType = fileExt;
    }

    const fileDir = path.join(storageDir,`${username}/texts/${file}`);
    fs.readdir(path.join(storageDir,`${username}/texts/`), (err,files) =>{
        if(files.includes(file)){
            fs.readFile(fileDir,(err,data) =>{
                if(err)
                {
                    throw err
                }
                insertLog(adminName,token,file,"download");
                return res.status(200)
                .contentType(mime.lookup(fileType))
                .setHeader('File-Type','text')
                .send(data);
            });
        } else { return res.status(404).send("File not found"); }
    })
})

textRouter.post('/', upload.single('file'),(req,res) =>{
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
    const[fName, fExt] = fileName.split('.'); 

    const fileBuffer = req.file.buffer;
    var duplicatedNum = 0;
    fs.readdir(path.join(storageDir,`${username}/texts/`),(err,files) =>{
        while(files.includes(fileName))
        {
            duplicatedNum++;
            fileName = `${fName}(${duplicatedNum}).${fExt}`;
        }

        const saveDir = path.join(storageDir,`${username}/texts/${fileName}`);
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

textRouter.delete('/',(req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    const file = req.query.file;
    const username = req.query.username;
    const targetDir = path.join(storageDir,`${username}/texts/${file}`);
  
    fs.unlink(targetDir, err =>{
        if(err)
        {
            return res.status(500)
            .send('Error while deleting the file. \n')
        }
        insertLog(username,token,file,"delete");  
        res.status(200)
        .send(`File deleted successfully \n`)
    })      
})

textRouter.patch('/',upload.single('file'),async (req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }
    const targetFile = req.query.file;
    const fileBuffer = req.file.buffer;
    const username = req.query.username;
    const [fileName,fileExt] = req.query.file.split('.');
    const fileType = mime.lookup(fileExt);
    let fileString = await typemap.get(fileType)(fileBuffer);
    const targetDir = path.join(storageDir,`${username}/texts`)

    fs.readdir(path.join(storageDir,`${username}/texts/`),(err,files) =>{
        
        if(err)
        {
            res.status(500).send("Error while reading the directory where the file is located.");
        }

        if(files.includes(targetFile))
        {
            fs.writeFile(path.join(targetDir,targetFile), fileString, (err) =>{
                if(err)
                {
                    res.status(500)
                    .send('Error while updating the file. \n');
                }
                insertLog(username,token,targetFile,"update");
                res.status(200)
                .send('File updated correctly. \n')
            })
        } 
        else 
        { 
            res.status(504)
            .send(`File ${targetFile} not found for updating.`) 
        }
    })
})

module.exports.textRouter = textRouter;
