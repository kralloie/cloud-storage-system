const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const bodyParser = require('body-parser');
const process = require('node:process');
const { imagesRouter } = require('./routers/images.js')
const { textRouter } = require('./routers/text.js')
const { validatePort } = require('./tools/validateparam.js');

var portParam = process.argv[2];


var PORT = validatePort(portParam);
const configPath = path.join(__dirname,'/config/serverconfig.json');
const app = express();
const upload = multer();
const configBuffer = fs.readFileSync(configPath);
const configString = configBuffer.toString('utf-8');
const configJSON = JSON.parse(configString);
configJSON.PORT = PORT;
fs.writeFileSync(configPath,JSON.stringify(configJSON));

app.use(bodyParser.urlencoded({extended: false}));
app.use(express.json());
app.use('/images',imagesRouter);
app.use('/texts',textRouter);

app.post('/', upload.single('file'),(req,res) =>{
    res.status(400).send('Please specify the type of file you are trying to upload in the URL path. Example: /images, /texts \n')
})

app.get('/',(req,res) =>{
    console.log('get request'); //debug
    try
    {
        const storageJSON = {
            images: [],
            texts: []
        }
        storageJSON.texts = fs.readdirSync(path.join(path.dirname(__filename),"./storage/texts"));
        storageJSON.images = fs.readdirSync(path.join(path.dirname(__filename),"./storage/images"));
        res.status(200)
        .setHeader('Connection-State','connected')
        .json(storageJSON);
    }
    catch(err){
        console.log(err);
        res.status(504)
        .send('Server error');
    }
})


app.listen(PORT, () =>{
    console.log(`Listening on port: ${PORT}`);
})

module.exports.PORT = PORT;
