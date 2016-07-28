package main

type Relay struct {

	// Connection list
	Connections map[*Connection]bool

	// Broadcast data buffer
	Broadcast chan []interface{}

	// Register requests from the connections.
	Register chan *Connection

	// Unregister requests from connections.
	Unregister chan *Connection
}

func (r *Relay) Run() {
	for {
		select {
		case c := <-r.Register:
			r.Connections[c] = true
		case c := <-r.Unregister:
			if _, ok := r.Connections[c]; ok {
				delete(r.Connections, c)
				close(c.Send)
			}
		case d := <-r.Broadcast:
			for c := range r.Connections {
				if c.session == d[0].(string) {
					select {
					case c.Send <- d[1].([]byte):
					default:
						close(c.Send)
						delete(r.Connections, c)
					}
				}
			}
		}
	}
}
