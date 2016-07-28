package main

import (
	"fmt"
	"github.com/gorilla/websocket"
	"time"
)

var curId int = 0

const (
	writeWait      = 10 * time.Second
	pongWait       = 60 * time.Second
	pingPeriod     = (pongWait * 9) / 10
	maxMessageSize = 32768
)

type Connection struct {
	Ws      *websocket.Conn
	Send    chan []byte
	id      int
	session string
}

func NewConnection(ws *websocket.Conn, ses string) *Connection {
	conn := &Connection{
		Ws:      ws,
		Send:    make(chan []byte, 100),
		id:      curId,
		session: ses,
	}
	curId++
	return conn
}

/*
 * Since we are going to use wesocket only
 * to broadcast the processed frame to client then we don't need
 * to implement the read handler
 */
func (c *Connection) ListenRead() {
	fmt.Println("Unimplement handler")
}

func (c *Connection) write(mt int, payload []byte) error {
	c.Ws.SetWriteDeadline(time.Now().Add(writeWait))
	return c.Ws.WriteMessage(mt, payload)
}

func (c *Connection) ListenWrite() {
	ticker := time.NewTicker(pingPeriod)
	defer func() {
		ticker.Stop()
		c.Ws.Close()
	}()
	for {
		select {
		case message, ok := <-c.Send:
			if !ok {
				c.write(websocket.CloseMessage, []byte{})
				return
			}
			if err := c.write(websocket.BinaryMessage, message); err != nil {
				return
			}
		case <-ticker.C:
			if err := c.write(websocket.PingMessage, []byte{}); err != nil {
				return
			}
		}
	}
}
