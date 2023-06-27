function validatePort(portParam,PORT)
{
    if(!portParam)
    {
        return 8080;
    }
    else
    {
        if(portParam.length <= 5)
        {
            const regex = new RegExp("^[0-9]*$");
            const match = regex.test(portParam);
            if(match)
            {
                if(portParam < 65535 && portParam > 1024)
                {
                    return portParam;
                }
            }
        }
        return 8080;
    }
}

module.exports.validatePort = validatePort;