/*
 *  Simple http server to accept stream data and broadcast
 *  the processed stream to client using websocket
 *
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com>
 *
 *  This source code is part of the tutorial
 *  in https://github.com/farisais/online-vidstream-processing
 */

package main

import (
	"io"
	"net/http"
)

var relay = Relay{
	Connections: make(map[*Connection]bool),
	Broadcast:   make(chan []byte),
	Register:    make(chan *Connection),
	Unregister:  make(chan *Connection),
}

func receiveStream(w *http.ResponseWriter, r *http.Request) {

	nBytes, nChunks := int64(0), int64(0)
	reader := bufio.NewReader(r.Body)

	/*
	 * Store the data in temp buffer
	 */
	buf := make([]byte, 0, 40*1024)
	for {
		n, err := reader.Read(buf[:cap(buf)])
		if n > cap(buf) {
			fmt.Println("Buffer overflow")
		}
		buf = buf[:n]
		if n == 0 {
			if err == nil {
				continue
			}
			if err == io.EOF {
				break
			}
			log.Fatal(err)
		}
		nChunks++
		nBytes += int64(len(buf))
		if err != nil && err != io.EOF {
			log.Fatal(err)
		}
		relay.Broadcast <- buf
	}
}

func listen(ws *websocket.Conn) {
	defer func() {
		err := ws.Close()
		if err != nil {
			s.errCh <- err
		}
	}()

	conn := NewConnection(ws)
	relay.Register <- conn
	conn.ListenWrite()
}

func main() {

	/*
	 * Registering http handler
	 */
	http.HandleFunc("/vstream", receiveStream)
	http.Handlefunc("/listen", websocket.Handler(listen))

	/*
	 * Running the relay process
	 */
	go relay.Run()

	/*
	 * Start the http server
	 */
	err := http.ListenAndServe(":8080", nil)
	if err != nil {
		log.Fatal("Error while running http server: ", err)
	}
}
