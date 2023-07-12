const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const bodyParser = require('body-parser');
const process = require('node:process');
const mysql = require('mysql')
const { imagesRouter } = require('./routers/images.js')
const { textRouter } = require('./routers/text.js')
const { validatePort } = require('./tools/validateparam.js');
const { randomUUID } = require('crypto');
let { tokens } = require('./token.js')

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

const dbconnection = mysql.createConnection({
    "user":'admin',
    "host":'localhost',
    "password":'adminpass',
    "database":'cloudservicedb'
})

dbconnection.connect();

app.get('/',(req,res) =>{
    res.status(200)
    .setHeader("Connection-state","connected")
    .end();
})

app.post('/logout',(req,res) =>{
    console.log(tokens);
    const queryToken = req.query.token;
    if(!tokens.includes(queryToken))
    {
        res.status(504)
        .end();
        return;
    }
    const requestJSON = req.body;
    const token = requestJSON.token;
    const username = requestJSON.username;
    const tokenIndex = tokens.indexOf(token);
    tokens.splice(tokenIndex,1);
    res.status(200)
    .end();
    console.log(tokens);
})

app.get('/global',(req,res) =>{
    try
    {
        const storageJSON = {
            users: []
        }
        const users = fs.readdirSync(path.join(path.dirname(__filename),"./storage"));
        users.forEach(user =>{
            const userStorageImages = fs.readdirSync(path.join(path.dirname(__filename),`./storage/${user}/images`));
            const userStorageTexts = fs.readdirSync(path.join(path.dirname(__filename),`./storage/${user}/texts`));
            storageJSON.users.push({
                "user":user,
                "images":userStorageImages,
                "texts":userStorageTexts
            })
        })
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

app.post('/login',async (req,res) =>{
    const JSONData = req.body;
    let username = JSONData.username;
    let password = JSONData.password;
    console.log(username,password);
    const matchArray = await new Promise((resolve,reject) =>{
        dbconnection.query("SELECT * FROM userCredentials WHERE username = ? AND password = ?;",[username,password],(err,rows) =>{
            if(err)
            {
                console.log(err);
            }
            resolve(rows[0]);
        })
    })
    if(!matchArray)
    {
        res.status(504)
        .json({
            "statusMessage":"Incorrect username or password.",
        });
    }
    else 
    {
        let tokenuuid = randomUUID();
        tokens.push(tokenuuid);
        var isAdminVal = (matchArray.adminPrivileges) ? true : false;
        console.log(matchArray);
        res.status(200)
        .json({
            "username":matchArray.username,
            "isAdmin":isAdminVal,
            "userId":matchArray.id,
            "statusMessage":"Logged in successfully",
            "token":tokenuuid
        });
    }
})

app.get('/user',(req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        console.log("invalid token")
        res.status(504)
        .send("Invalid token")
        return;
    }
    const username = req.query.username;
    let userStorageImages;
    let userStorageTexts;

    try
    {
        userStorageImages = fs.readdirSync(path.join(path.dirname(__filename),`./storage/${username}/images`));
        userStorageTexts = fs.readdirSync(path.join(path.dirname(__filename),`./storage/${username}/texts`));
    }
    catch(err)
    {
        console.log(err);
        res.status(504)
        .send("Server error");
    }

    const storageJSON = {
        "images": userStorageImages,
        "texts": userStorageTexts
    }

    res.status(200)
    .json(storageJSON);
})

app.get('/admin',async (req,res) =>{
    const userCredentialsTable = await new Promise((resolve,reject) =>{
        dbconnection.query('SELECT * FROM userCredentials',(err,rows) =>{
            resolve(rows);
        })
    })
    res.status(200)
    .json(userCredentialsTable);
})

app.post('/register',(req,res) =>{
    const requestJSON = req.body;
    const username = requestJSON.username;
    const password = requestJSON.password;
    dbconnection.query(`INSERT INTO userCredentials(username,password) VALUES("${username}","${password}")`, (err) =>{
        if(err)
        {
            res.status(505)
            .send("Username already in use, please input a different one.");
            return;
        }
        createUserStorage(username);
        res.status(200)
        .send("Account created successfully.");
    })
})

app.get('/logs', async (req,res) =>{
    const token = req.query.api;
    if(!tokens.includes(token))
    {
        res.status(504)
        .send("Invalid token")
        return;
    }

    const logObject = await new Promise((resolve,reject) =>{
        dbconnection.query("SELECT * FROM logs", (err,rows) =>{
            if(err) { throw err };
            resolve(rows);
        })
    })
    res.status(200)
    .json(logObject);
})

function createUserStorage(username)
{
    fs.mkdir(`./storage/${username}`, (err) =>{
        if (err) throw err;
        fs.mkdir(`./storage/${username}/texts`, (err) => { if (err) throw err;});
        fs.mkdir(`./storage/${username}/images`, (err) => { if (err) throw err});
    })
}

app.listen(PORT, () =>{
    console.log(`Listening on port: ${PORT}`);
})

module.exports.PORT = PORT;
