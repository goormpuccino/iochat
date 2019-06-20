var express = require('express');
var app = express();
var server = require('http').createServer(app);
var io = require('socket.io')(server);
var port = 3000;

app.use(function(req, res, next) {
  req.connection.setNoDelay(true);
  next();
});

console.log("Server Running!");

//users = [];
//connections = [];

/* database */
/*
var mysql      = require('mysql');
var connection = mysql.createConnection({
  host     : 'localhost',
  user     : 'root',
  password : 'qw12143286',
  port     : 3306,
  database : 'myLogin'
});

connection.connect();
console.log("database connected");
*/

/* server */
var child;
server.listen(process.env.PORT || port, function() {
  console.log('Server listening at port ' + port);

  var spawn = require('child_process').spawn;
  child = spawn('/data/iochat/inputsync', [], {shell: true} );
child.stdout.setEncoding('utf8');
child.stdout.on('data', function(data) {
    console.log('stdout: ' + data);
});

child.stderr.setEncoding('utf8');
child.stderr.on('data', function(data) {
    console.log('stderr: ' + data);
});

child.on('close', function(code) {
    console.log('closing code: ' + code);
});

});
//
// app.use(express.static(__dirname + '/public'));

/* server get html - redirections */
app.get('/test_key.html', function(req, res) {
  console.log("test page");
  res.sendFile(__dirname + '/decoding.html');
});

app.get('/', function(req, res) {
  console.log("redireciton");
  res.writeHead(302, { 'Location' : '/index.html'});
  res.end();
});

app.get('/index.html', function(req, res) {
  console.log("login page");
  res.sendFile(__dirname + '/login_page.html');
});
//
// app.get('/loginStyle.css', function(req, res) {
//   res.sendFile(__dirname + '/loginStyle.css');
// });

app.get('/stream.html', function(req, res) {
  console.log("stream page");
  res.sendFile(__dirname + '/stream.html');
});

/*
app.get('/login.html', function(req, res) {
  res.sendFile(__dirname + '/loading.html');
});
*/

/* server connection handlers */
io.sockets.on('connection', function(socket){
  //connections.push(socket);
  //console.log('Connected : %s sockets connected', connections.length);
  var windowx = 1024;
  var windowy = 768;

  //Disconnect
  socket.on('disconnect', function(data){
    //connections.splice(connections.indexOf(socket), 1);
    //console.log('Disconnected : %s sockets connected', connections.length);
  });

  //메세지 받는 함
  socket.on('send message', function(data){
    console.log(data);
    if (data.startsWith('Mouse Dragging')) {
      var split = data.split(" ");
      var send = '0' + String(parseInt(Number(split[3]) * 4096 / windowx)).padStart(8, "0") + ' ' + String(parseInt(Number(split[5]) * 4096 / windowy)).padStart(8, "0");
      console.log('TOSEND: ' + send);
      //stdinStream.push(send);  // Add data to the internal queue for users of the stream to consume
      //stdinStream.push(null);
      //stdinStream.pipe(child.stdin);
      child.stdin.write(send);
      console.log('pipe');
    } else if (data.startsWith('Mouse Clicked')) {
      var send = "110000000000000000";
      child.stdin.write(send);
      console.log('pipe');

      var split = data.split(" ");
      send = '0' + String(parseInt(Number(split[3]) * 4096 / windowx)).padStart(8, "0") + ' ' + String(parseInt(Number(split[5]) * 4096 / windowy)).padStart(8, "0");
      child.stdin.write(send);
      console.log('pipe');
    } else if (data.startsWith('Mouse Up')) {
      var send = "100000000000000000";
      child.stdin.write(send);
      console.log('pipe');
    } else if (data.startsWith('Window Resized')) {
      var split = data.split(" ");
      windowx = Number(split[3]);
      windowy = Number(split[5]);

      send = '2' + String(parseInt(Number(split[3]))).padStart(8, "0") + ' ' + String(parseInt(Number(split[5]))).padStart(8, "0");
      console.log('TOSEND: ' + send);
      child.stdin.write(send);

      console.log('New window size recorded');
    } else if (data.startsWith('keydown')) {
      var split = data.split(" ");
      var send = '3000000000' + String(parseInt(Number(split[1]))).padStart(8, "0");
      child.stdin.write(send);
      console.log('pipe');
    } else if (data.startsWith('keyup')) {
      var split = data.split(" ");
      var send = '3100000000' + String(parseInt(Number(split[1]))).padStart(8, "0");
      child.stdin.write(send);
      console.log('pipe');
    }
//    child.stdin.write('\n');

    socket.emit('new message', data);
  });

  socket.on('get name', function(data) {
    /*
    console.log('received data: ' + data);
    connection.query('SELECT name from Members where id=\'' + data + '\';', function(err, result, fields) {
      if (!err) {
        console.log('database match : ' + result[0].name);

        //socket.emit('redirect', '/login.html');
        socket.emit('login_success', result[0].name);

        setTimeout(function() {
          socket.emit('loading', 'done');
        }, 1000);
      }
      else {
        console.log(err);
      }
    });
    */

    socket.emit('login_success', data);

    setTimeout(function() {
	socket.emit('loading', 'done');
    }, 2000);
  });



});

/*

io.close();
server.close();
connection.end();

*/
