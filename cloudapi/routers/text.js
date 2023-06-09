const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const { typemap } = require('../tools/typehandlermap');

const textRouter = express.Router();
const textStorageDir = './storage/texts/'
const upload = multer();

textRouter.get('/',(req,res) =>{
    const file = req.query.file;
    const fileExt = path.extname(file);
    const fileDir = path.join(textStorageDir,file);
    fs.readdir(textStorageDir, (err,files) =>{
        if(files.includes(file)){
            fs.readFile(fileDir,(err,data) =>{
                if(err)
                {
                    throw err
                }
                return res.status(200).contentType(mime.lookup(fileExt)).setHeader('File-Type','text').send(data);
            });
        } else { return res.status(404).send('File not found \n'); }
    })
})

textRouter.post('/', upload.single('file'),(req,res) =>{
    console.log('Post request received');
    var fileName = req.query.file;
    const [fName, fExt] = fileName.split('.');
    const fileBuffer = req.file.buffer;
    var duplicatedNum = 0;
    fs.readdir(textStorageDir,(err,files) =>{
        while(files.includes(fileName))
        {
            duplicatedNum++;
            fileName = `${fName}(${duplicatedNum}).${fExt}`;
        }

        const saveDir = path.join(textStorageDir,fileName);
        fs.writeFile(saveDir, fileBuffer, (err) =>{
            if(err)
            {
               return res.status(500).send('Error while saving the file. \n');
            }
            return res.status(200).send('File saved correctly \n');
        })
    })
})

textRouter.delete('/',(req,res) =>{
    const file = req.query.file;
    const targetDir = path.join(textStorageDir, file);
  
    fs.unlink(targetDir, err =>{
        if(err)
        {
            return res.status(500).send('Error while deleting the file. \n')
        }

        res.status(200).send(`File deleted successfully \n`)
    })        
})

textRouter.patch('/',upload.single('file'),async (req,res) =>{
    const targetFile = req.query.file;
    const fileBuffer = req.file.buffer;
    const [fileName,fileExt] = req.query.file.split('.');
    const fileType = mime.lookup(fileExt);
    //let fileString = await typemap.get(fileType)(fileBuffer);
    let fileString = fileBuffer.toString();
    try
    {
        const JSONFile = JSON.parse(fileString);
        fileString = JSON.stringify(JSONFile,null,4);
    }
    catch(err){
        console.log(err);
    }

    fs.readdir(textStorageDir,(err,files) =>{
        
        if(err)
        {
            res.status(500).send("Error while reading the directory where the file is located.");
        }

        if(files.includes(targetFile))
        {
            fs.writeFile(path.join(textStorageDir,targetFile), fileString, (err) =>{
                if(err)
                {
                    res.status(500).send('Error while updating the file. \n');
                }
                res.status(200).send('File updated correctly. \n')
            })
        } else { res.status(504).send(`File ${targetFile} not found for updating.`) }
    })
})

module.exports.textRouter = textRouter;
