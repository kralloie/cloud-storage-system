const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const bodyParser = require('body-parser');
const { imagesRouter } = require('./routers/images.js')
const { textRouter } = require('./routers/text.js')

const app = express();
const upload = multer();
const PORT = 8080;

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
        res.status(200).json(storageJSON);
    }
    catch(err){
        console.log(err);
        res.status(504).send('Server error');
    }
})


app.listen(PORT, () =>{
    console.log(`Listening on port: ${PORT}`);
})
