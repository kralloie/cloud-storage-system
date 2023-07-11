let { tokens } = require('../token');
const mysql = require('mysql');


const dbconnection = mysql.createConnection({
    "user":'admin',
    "host":'localhost',
    "password":'adminpass',
    "database":'cloudservicedb'
})

dbconnection.connect();

function insertLog(username,token,filename,action)
{
    if(!tokens.includes(token)) { return };
    const currentDate = new Date();
    const dateISOString = currentDate.toISOString();
    let [date,hour] = dateISOString.split("T");
    hour = hour.substring(0,7)
    dbconnection.query(`INSERT INTO logs(action,hour,day,file,user) VALUES("${action}","${hour}","${date}","${filename}","${username}")`, (err) =>{
        if(err) {throw err};
        console.log("Log created");
    })
}

module.exports.insertLog = insertLog