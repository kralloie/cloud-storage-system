const express = require('express')
const fs = require('fs');
const mime = require('mime-types')
const path = require('path')
const multer = require('multer')
const mysql = require('mysql')

const loginRouter = express.Router()
const upload = multer();

const dbconnection = mysql.createConnection({
    "user":'admin',
    "host":'localhost',
    "password":'adminpass',
    "database":'cloudservicedb'
})

dbconnection.connect();

loginRouter.post('/',async (req,res) =>{
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
        var isAdminVal = (matchArray.adminPrivileges) ? true : false;
        console.log(matchArray);
        res.status(200)
        .json({
            "username":matchArray.username,
            "isAdmin":isAdminVal,
            "userId":matchArray.id,
            "statusMessage":"Logged in successfully",
        });
    }
})

module.exports.loginRouter = loginRouter;