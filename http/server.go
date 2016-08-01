/*
 *  Simple http server to accept stream data and broadcast
 *  the processed stream to client using websocket
 *
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com>
 *
 *  This source code is part of the tutorial
 *  in https://github.com/farisais/online-videostream-processing
 */

package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"github.com/gorilla/websocket"
	"io"
	"net/http"
	"runtime"
	"sync"
)

const (
	CPU  = 2
	addr = "localhost"
	port = "8080"
)

var lock sync.Mutex

/*
 * Relay for switching packet from inconnection to websocket
 */
var relay = Relay{
	Connections: make(map[*Connection]bool),
	Broadcast:   make(chan []interface{}),
	Register:    make(chan *Connection),
	Unregister:  make(chan *Connection),
}

/*
 * Codec to use to decode the frame
 */
var codec *string

/*
 * Processing algorithm
 */
var processAlg *string

/*
 * Pointer ref to the current avprocess interface
 */
var AVObjPtr *AvProcessInterface

type StreamEndpoint struct {
	session        string
	receiverBuffer chan []byte
	avInterface    *AvProcessInterface
	procInterface  *ProcessInterface
}

var InConnections []*StreamEndpoint

func receiveStream(w http.ResponseWriter, r *http.Request) {

	secret, ok := r.URL.Query()["secret"]

	/*
	 * Check if already session running
	 */

	if exist := isSessionExist(secret[0]); ok && !exist {
		conn := &StreamEndpoint{
			session:        secret[0],
			receiverBuffer: make(chan []byte, 100),
			avInterface:    NewAvInterface(codec, secret[0]),
			procInterface:  NewProcInterface(processAlg),
		}

		InConnections = append(InConnections, conn)

		/*
		 * Setup chain pipe processing
		 */
		go conn.avInterface.RunDecoder(&conn.receiverBuffer)
		go conn.procInterface.RunProcessing(&conn.avInterface.PipeOutDecoder)
		go conn.avInterface.RunEncoder(&conn.procInterface.PipeOut)

		go func() {
			for d := range conn.avInterface.PipeOutEncoder {
				relay.Broadcast <- []interface{}{conn.session, d}
			}
		}()

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
					fmt.Println("End of file reach ...")
					break
				}
				fmt.Println(err)
			}
			nChunks++
			nBytes += int64(len(buf))
			if err != nil && err != io.EOF {
				fmt.Println(err)
			}
			conn.receiverBuffer <- buf
		}

		/*
		 * Remove in connection
		 */
		removeConFromList(conn)
	} else {
		io.WriteString(w, "Secret is not defined")
	}
}

func removeConFromList(con *StreamEndpoint) {
	var index int = -1
	for i, con := range InConnections {
		if con.session == con.session {
			index = i
		}
	}
	if index >= 0 {
		fmt.Println("Removing connection from relay : ", con.session)
		InConnections = append(InConnections[:index], InConnections[index+1:]...)
	}
}

var upgrader = websocket.Upgrader{
	ReadBufferSize:  32768,
	WriteBufferSize: 32768,
	CheckOrigin:     func(r *http.Request) bool { return true }, // Accept all origin
}

func listen(w http.ResponseWriter, r *http.Request) {
	secret, ok := r.URL.Query()["secret"]
	/*
	 * Check if secret is defined
	 */
	if ok {
		/*
		 * updgrade the current http connection to websocket
		 */
		ws, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			fmt.Println(err)
			return
		}

		defer func() {
			err := ws.Close()
			if err != nil {
				fmt.Printf("error: %v", err)
			}
		}()

		conn := NewConnection(ws, secret[0])
		relay.Register <- conn

		/*
		 * Sending initial frame required by jsmpg
		 */
		var width uint16 = 640
		var height uint16 = 480
		prefixCode := []byte("jsmp")

		buf := new(bytes.Buffer)
		binary.Write(buf, binary.BigEndian, prefixCode)
		binary.Write(buf, binary.BigEndian, width)
		binary.Write(buf, binary.BigEndian, height)
		conn.Send <- buf.Bytes()

		conn.ListenWrite()
	}
}

func main() {
	runtime.GOMAXPROCS(CPU)

	/*
	 * Parsing flag
	 */
	codec = flag.String("codec", "h264", "Codec to use to decode frame")
	processAlg = flag.String("alg", "", "Processing algorithm")
	flag.Parse()

	fmt.Println(".")
	/*
	 * Registering http handler
	 */
	http.HandleFunc("/vstream", receiveStream)
	http.HandleFunc("/listen", listen)

	fmt.Println(".")
	/*
	 * Running the relay process
	 */
	go relay.Run()

	/*
	 * Start the http server
	 */
	fmt.Printf(">>> Running server on %s:%s <<<\n", addr, port)
	err := http.ListenAndServe(addr+":"+port, nil)
	if err != nil {
		fmt.Println("Error while running http server: ", err)
	}

}

func isSessionExist(ses string) bool {
	for _, con := range InConnections {
		if con.session == ses {
			return true
		}
	}
	return false
}
