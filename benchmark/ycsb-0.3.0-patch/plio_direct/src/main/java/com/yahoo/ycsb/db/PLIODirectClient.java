package com.yahoo.ycsb.db;

import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.Vector;

import com.yahoo.ycsb.DB;
import com.yahoo.ycsb.DBException;
import com.yahoo.ycsb.ByteIterator;
import com.yahoo.ycsb.StringByteIterator;

import edu.cuhk.cse.plio.PLIODirect;

public class PLIODirectClient extends DB {
	// Properties
	public static final String HOST_PROPERTY = "plio.host";
	public static final String PORT_PROPERTY = "plio.port";
	public static final String KEY_SIZE_PROPERTY = "plio.key_size";
	public static final String CHUNK_SIZE_PROPERTY = "plio.chunk_size";
	public static final String SERVER_COUNT_PROPERTY = "plio.server_count";
	// Return values
	public static final int OK = 0;
	public static final int ERROR = -1;
	public static final int NOT_FOUND = -2;

	private PLIODirect plio;

	public void init() throws DBException {
		Properties props = getProperties();
		String s;
		int keySize, chunkSize, serverCount;
		String[] hosts;
		int[] ports;

		s = props.getProperty( SERVER_COUNT_PROPERTY );
		serverCount = s != null ? Integer.parseInt( s ) : 1;

		hosts = new String[ serverCount ];
		ports = new int[ serverCount ];

		for ( int i = 0; i < serverCount; i++ ) {
			hosts[ i ] = props.getProperty( HOST_PROPERTY.concat( Integer.toString( i ) ) );

			s = props.getProperty( PORT_PROPERTY.concat( Integer.toString( i ) ) );
			ports[ i ] = s != null ? Integer.parseInt( s ) : PLIODirect.DEFAULT_PORT;

			if ( hosts[ i ] == null )
				throw new DBException( "Invalid host name for server #" + i + "." );
		}

		s = props.getProperty( KEY_SIZE_PROPERTY );
		keySize = s != null ? Integer.parseInt( s ) : PLIODirect.DEFAULT_KEY_SIZE;

		s = props.getProperty( CHUNK_SIZE_PROPERTY );
		chunkSize = s != null ? Integer.parseInt( s ) : PLIODirect.DEFAULT_CHUNK_SIZE;

		int fromId = ( int ) ( Math.random() * Integer.MAX_VALUE );
		int toId = ( int ) ( Math.random() * Integer.MAX_VALUE );
		if ( fromId > toId ) {
			int tmp = fromId;
			fromId = toId;
			toId = tmp;
		}

		plio = new PLIODirect( keySize, chunkSize, hosts, ports, fromId, toId );

		if ( ! plio.connect() )
			throw new DBException();
	}

	public void cleanup() throws DBException {
		plio.disconnect();
	}

	/* Calculate a hash for a key to store it in an index.  The actual return
	* value of this function is not interesting -- it primarily needs to be
	* fast and scattered along the whole space of doubles.  In a real world
	* scenario one would probably use the ASCII values of the keys.
	*/
	private double hash( String key ) {
		return key.hashCode();
	}

	@Override
	public int read( String table, String key, Set<String> fields, HashMap<String, ByteIterator> result ) {
		int ret = OK;
		for ( String f : fields ) {
			String value = plio.get( table + ":" + key + ":" + f );
			if ( value == null )
				ret = ERROR;
			result.put( f, new StringByteIterator( value ) );
		}
		return ret;
	}

	@Override
	public int insert( String table, String key, HashMap<String, ByteIterator> values ) {
		int ret = OK;
		for ( Map.Entry<String, ByteIterator> entry : values.entrySet() ) {
			if ( ! plio.set( table + ":" + key + ":" + entry.getKey(), entry.getValue().toString() ) )
				ret = ERROR;
		}
		return ret;
	}

	@Override
	public int delete( String table, String key ) {
		return plio.delete( key ) ? OK : ERROR;
	}

	@Override
	public int update( String table, String key, HashMap<String, ByteIterator> values ) {
		int ret = OK;
		for ( Map.Entry<String, ByteIterator> entry : values.entrySet() ) {
			if ( ! plio.update( table + ":" + key + ":" + entry.getKey(), entry.getValue().toString(), 0 ) )
				ret = ERROR;
		}
		return ret;
	}

	@Override
	public int scan( String table, String startKey, int recordCount, Set<String> fields, Vector<HashMap<String, ByteIterator>> result ) {
		return ERROR;
	}

}