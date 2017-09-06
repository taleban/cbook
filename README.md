<b>Cbook is a simple facebook messenger client written in C compatible with only Linux</b>
<br>
<br>
<p>It needs various dependencies like<br><ul><li>Curl</li><li>libcurl dev</li><li>json-c</li><li>pthread</li></ul>
<br>
<p>This code is just a browser emulator that download contents from Facebook like html and json data</p>
<p>He ('cause is a man) can login, download cookies to automatically login next time</p>
<p>He can browse only last chats, you can <b>read, send and automatically sync for new messages</b>
<h3>It uses standard https features to be somehow secure (but I don't bet this)</h3>
<br>
<br>
<b>Json-c reposity: </b>https://github.com/json-c/json-c
<br>
<b>libcurl: </b>https://curl.haxx.se/libcurl/c/   <p>or just these commands: <code>apt-get install libcurl4-openssl-dev</code>   <code>apt-get install libcurl4-gnutls-dev</code>
<br>
<b>pthread: </b>https://computing.llnl.gov/tutorials/pthreads/
<br>
<h6>For Gcc compiling I use this command:<code>gcc -o test cbook.c -lcurl -ljson-c -lpthread</code></h6>
where 'test' is the name of the executable
