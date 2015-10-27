package edu.cuhk.cse.plio;

import java.net.Socket;
import java.net.UnknownHostException;

import java.io.BufferedInputStream;
import java.io.OutputStream;
import java.io.IOException;

public class PLIO {
	private Protocol protocol;
	private String host;
	private int port;
	private int id;
	private Socket socket;
	private BufferedInputStream in;
	private OutputStream out;
	private static final boolean isDebugMode = true;

	public static int DEFAULT_PORT = 9110;
	public static int DEFAULT_KEY_SIZE = 255;
	public static int DEFAULT_CHUNK_SIZE = 4096;

	public PLIO( int keySize, int chunkSize, String host, int port, int startId ) {
		this.protocol = new Protocol( keySize, chunkSize );
		this.host = host;
		this.port = port;
		this.id = startId;
	}

	public int nextVal() {
		if ( this.id == Integer.MAX_VALUE - 1 )
			this.id = 0;
		else
			this.id++;
		return this.id;
	}

	private int read( int size ) throws IOException {
		int recvBytes = 0, ret;
		do {
			ret = this.in.read( this.protocol.buf, recvBytes, size - recvBytes );
			if ( ret > 0 )
				recvBytes += ret;
		} while ( ret >= 0 && recvBytes < size );
		return recvBytes;
	}

	private void debug( String s ) {
		if ( isDebugMode )
			System.out.println( "\n" + s );
	}

	public boolean connect() {
		try {
			this.socket = new Socket( this.host, this.port );
		} catch( UnknownHostException e ) {
			System.err.println( "PLIO.connect(): [Error] Unknown host is specified." );
			System.exit( 1 );
		} catch( IOException e ) {
			System.err.println( "PLIO.connect(): [Error] Fail to connect." );
			System.exit( 1 );
		}

		try {
			this.in = new BufferedInputStream( this.socket.getInputStream() );
		} catch( IOException e ) {
			System.err.println( "PLIO.connect(): [Error] Unable to get socket's input stream." );
			System.exit( 1 );
		}
		try {
			this.out = this.socket.getOutputStream();
		} catch( IOException e ) {
			System.err.println( "PLIO.connect(): [Error] Unable to get socket's output stream." );
			System.exit( 1 );
		}

		// Prepare register message
		int id = this.nextVal();
		int bytes = this.protocol.generateHeader(
			Protocol.PROTO_MAGIC_REQUEST,
			Protocol.PROTO_MAGIC_TO_MASTER,
			Protocol.PROTO_OPCODE_REGISTER,
			0,
			id
		);
		try {
			this.out.write( this.protocol.buf, 0, bytes );
		} catch( IOException e ) {
			System.err.println( "PLIO.connect(): [Error] Unable to send register request to master." );
			System.exit( 1 );
		}

		try {
			bytes = this.read( Protocol.PROTO_HEADER_SIZE );
		} catch( IOException e ) {
			System.err.println( "PLIO.connect(): [Error] Unable to read response from master." );
			return false;
		}
		if ( bytes == Protocol.PROTO_HEADER_SIZE ) {
			this.protocol.parseHeader( bytes );
			if ( this.protocol.header.id != id ) {
				System.err.println( "PLIO.connect(): [Error] The response does not match the request ID." );
				return false;
			}
			return true;
			// this.debug( this.protocol.header.toString() );
		} else {
			System.err.println( "PLIO.connect(): [Error] Header length mismatch: " + bytes + " vs. " + this.protocol.header.length + "." );
			return false;
		}
	}

	public boolean disconnect() {
		try {
			this.socket.close();
		} catch( IOException e ) {
			return false;
		}
		return true;
	}

	public String get( String key ) {
		byte[] k = key.getBytes();
		return this.get( k, k.length );
	}

	public boolean set( String key, String value ) {
		byte[] k = key.getBytes();
		byte[] v = value.getBytes();
		return this.set( k, k.length, v, v.length );
	}

	public boolean update( String key, String value, int offset ) {
		byte[] k = key.getBytes();
		byte[] v = value.getBytes();
		return this.update( k, k.length, v, offset, v.length );
	}

	public boolean delete( String key ) {
		byte[] k = key.getBytes();
		return this.delete( k, k.length );
	}

	public String get( byte[] key, int keySize ) {
		int id = this.nextVal();
		int bytes = this.protocol.generateKeyHeader(
			Protocol.PROTO_MAGIC_REQUEST,
			Protocol.PROTO_MAGIC_TO_MASTER,
			Protocol.PROTO_OPCODE_GET,
			id,
			keySize, key
		);
		try {
			this.out.write( this.protocol.buf, 0, bytes );
		} catch( IOException e ) {
			System.err.println( "PLIO.get(): [Error] Unable to send GET request to master." );
			return null;
		}

		try {
			bytes = this.read( Protocol.PROTO_HEADER_SIZE );
		} catch( IOException e ) {
			System.err.println( "PLIO.get(): [Error] Unable to read GET response from master." );
			return null;
		}
		if ( bytes == Protocol.PROTO_HEADER_SIZE ) {
			this.protocol.parseHeader( bytes );
			// this.debug( this.protocol.header.toString() );
		}

		try {
			bytes = this.read( this.protocol.header.length );
		} catch( IOException e ) {
			System.err.println( "PLIO.get(): [Error] Unable to read GET response from master." );
			return null;
		}
		if ( bytes == this.protocol.header.length ) {
			if ( this.protocol.header.id != id ) {
				System.err.println( "PLIO.get(): [Error] The response does not match the request ID." );
			} else if ( this.protocol.header.isSuccessful() ) {
				this.protocol.parseKeyValueHeader( bytes, 0 );
				// this.debug( this.protocol.keyValueHeader.toString() );
				if ( this.protocol.keyValueHeader.match( key, keySize ) )
					return this.protocol.keyValueHeader.value();
				System.err.println( "PLIO.get(): [Error] The response does not match with the key." );
			} else {
				this.protocol.parseKeyHeader( bytes, 0 );
				// this.debug( this.protocol.keyHeader.toString() );
				// System.err.println( "PLIO.get(): [Error] Key not found." );
			}
		} else {
			System.err.println( "PLIO.get(): [Error] Header length mismatch: " + bytes + " vs. " + this.protocol.header.length + "." );
		}
		return null;
	}

	public boolean set( byte[] key, int keySize, byte[] value, int valueSize ) {
		int id = this.nextVal();
		int bytes = this.protocol.generateKeyValueHeader(
			Protocol.PROTO_MAGIC_REQUEST,
			Protocol.PROTO_MAGIC_TO_MASTER,
			Protocol.PROTO_OPCODE_SET,
			id,
			keySize, key,
			valueSize, value
		);
		try {
			this.out.write( this.protocol.buf, 0, bytes );
		} catch( IOException e ) {
			System.err.println( "PLIO.set(): [Error] Unable to send SET request to master." );
			return false;
		}

		try {
			bytes = this.read( Protocol.PROTO_HEADER_SIZE );
		} catch( IOException e ) {
			System.err.println( "PLIO.set(): [Error] Unable to read SET response from master." );
			return false;
		}
		if ( bytes == Protocol.PROTO_HEADER_SIZE ) {
			this.protocol.parseHeader( bytes );
			// this.debug( this.protocol.header.toString() );
		}

		try {
			bytes = this.read( this.protocol.header.length );
		} catch( IOException e ) {
			System.err.println( "PLIO.set(): [Error] Unable to read SET response from master." );
			return false;
		}
		if ( bytes == this.protocol.header.length ) {
			if ( this.protocol.header.id != id ) {
				System.err.println( "PLIO.set(): [Error] The response does not match the request ID." );
			} else if ( this.protocol.header.isSuccessful() ) {
				this.protocol.parseKeyHeader( bytes, 0 );
				// this.debug( this.protocol.keyHeader.toString() );
				if ( this.protocol.keyHeader.match( key, keySize ) ) {
					return true;
				} else {
					System.err.println( "PLIO.set(): [Error] The response does not match with the key." );
				}
			} else {
				System.err.println( "PLIO.set(): [Error] Request failed." );
			}
		} else {
			System.err.println( "PLIO.set(): [Error] Header length mismatch: " + bytes + " vs. " + this.protocol.header.length + "." );
		}
		return false;
	}

	public boolean update( byte[] key, int keySize, byte[] valueUpdate, int valueUpdateOffset, int valueUpdateSize ) {
		int id = this.nextVal();
		int bytes = this.protocol.generateKeyValueUpdateHeader(
			Protocol.PROTO_MAGIC_REQUEST,
			Protocol.PROTO_MAGIC_TO_MASTER,
			Protocol.PROTO_OPCODE_UPDATE,
			id,
			keySize, key,
			valueUpdateOffset, valueUpdateSize, valueUpdate
		);
		try {
			this.out.write( this.protocol.buf, 0, bytes );
		} catch( IOException e ) {
			System.err.println( "PLIO.update(): [Error] Unable to send UPDATE request to master." );
			return false;
		}

		try {
			bytes = this.read( Protocol.PROTO_HEADER_SIZE );
		} catch( IOException e ) {
			System.err.println( "PLIO.update(): [Error] Unable to read UPDATE response from master." );
			return false;
		}
		if ( bytes == Protocol.PROTO_HEADER_SIZE ) {
			this.protocol.parseHeader( bytes );
			// this.debug( this.protocol.header.toString() );
		}

		try {
			bytes = this.read( this.protocol.header.length );
		} catch( IOException e ) {
			System.err.println( "PLIO.update(): [Error] Unable to read UPDATE response from master." );
			return false;
		}
		if ( bytes == this.protocol.header.length ) {
			if ( this.protocol.header.id != id ) {
				System.err.println( "PLIO.update(): [Error] The response does not match the request ID." );
			} else if ( this.protocol.header.isSuccessful() ) {
				this.protocol.parseKeyValueUpdateHeader( bytes, 0 );
				// this.debug( this.protocol.keyValueUpdateHeader.toString() );
				if ( this.protocol.keyValueUpdateHeader.match( key, keySize ) )
					return true;
				else
					System.err.println( "PLIO.update(): [Error] The response does not match with the key." );
			} else {
				System.err.println( "PLIO.update(): [Error] Request failed." );
			}
		} else {
			System.err.println( "PLIO.update(): [Error] Header length mismatch: " + bytes + " vs. " + this.protocol.header.length + "." );
		}
		return false;
	}

	public boolean delete( byte[] key, int keySize ) {
		int id = this.nextVal();
		int bytes = this.protocol.generateKeyHeader(
			Protocol.PROTO_MAGIC_REQUEST,
			Protocol.PROTO_MAGIC_TO_MASTER,
			Protocol.PROTO_OPCODE_DELETE,
			id,
			keySize, key
		);
		try {
			this.out.write( this.protocol.buf, 0, bytes );
		} catch( IOException e ) {
			System.err.println( "PLIO.delete(): [Error] Unable to send DELETE request to master." );
			return false;
		}

		try {
			bytes = this.read( Protocol.PROTO_HEADER_SIZE );
		} catch( IOException e ) {
			System.err.println( "PLIO.delete(): [Error] Unable to read DELETE response from master." );
			return false;
		}
		if ( bytes == Protocol.PROTO_HEADER_SIZE ) {
			this.protocol.parseHeader( bytes );
			// this.debug( this.protocol.header.toString() );
		}

		try {
			bytes = this.read( this.protocol.header.length );
		} catch( IOException e ) {
			System.err.println( "PLIO.delete(): [Error] Unable to read DELETE response from master." );
			return false;
		}
		if ( bytes == this.protocol.header.length ) {
			if ( this.protocol.header.id != id ) {
				System.err.println( "PLIO.delete(): [Error] The response does not match the request ID." );
			} else if ( this.protocol.header.isSuccessful() ) {
				this.protocol.parseKeyHeader( bytes, 0 );
				// this.debug( this.protocol.keyHeader.toString() );
				if ( this.protocol.keyHeader.match( key, keySize ) ) {
					return true;
				} else {
					System.err.println( "PLIO.delete(): [Error] The response does not match with the key." );
				}
			} else {
				System.err.println( "PLIO.delete(): [Error] Request failed." );
			}
		} else {
			System.err.println( "PLIO.delete(): [Error] Header length mismatch: " + bytes + " vs. " + this.protocol.header.length + "." );
		}
		return false;
	}
}