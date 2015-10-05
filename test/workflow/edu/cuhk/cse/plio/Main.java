package edu.cuhk.cse.plio;

import java.util.Scanner;
import java.util.HashMap;
import java.util.Random;
import java.util.Map;
import java.util.Set;

public class Main implements Runnable {
	/* Configuration */
	public static int keySize, chunkSize, port;
	public static String host;
	/* Test parameters */
	public static int numThreads, numRecords, numOps;
	public static boolean fixedSize = true;
	/* States */
	public static Main[] mainObjs;
	public static Thread[] threads;
	public static int completedOps;
	public static Object lock;
	/* Constants */
	private static final String characters = "01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	private static final int charactersLength = characters.length();

	/* Instance variables */
	private PLIO plio;
	private HashMap<String, String> map;
	private Random random;
	public int[] completed, succeeded;

	public Main( int startId ) {
		this.plio = new PLIO( Main.keySize, Main.chunkSize, Main.host, Main.port, startId );
		this.map = new HashMap<String, String>( Main.numRecords / Main.numThreads );
		this.completed = new int[ 4 ];
		this.succeeded = new int[ 4 ];
		this.random = new Random();
	}

	private String generate( int size ) {
		StringBuilder sb = new StringBuilder( size );
		for ( int i = 0; i < size; i++ ) {
			sb.append( Main.characters.charAt( this.random.nextInt( Main.charactersLength ) ) );
		}
		return sb.toString();
	}

	private int incrementCounter() {
		int ret;
		synchronized( Main.lock ) {
			this.completedOps++;
			ret = this.completedOps;
			if ( this.completedOps % ( Main.numOps / 10 ) == 0 )
				System.out.printf( "Completed operations: %d (%.2f%%)\r", this.completedOps, ( double ) this.completedOps / Main.numOps * 100.0 );
		}
		return ret;
	}

	public void run() {
		if ( ! plio.connect() )
			return;

		int rand, size;

		int i = 0, keySize, valueSize, index,
		    numOps = Main.numOps / Main.numThreads,
		    numRecords = Main.numRecords / Main.numThreads;
		String key, value;
		boolean ret;

		while( i < numOps ) {
			rand = this.random.nextInt( 4 );
			size = this.map.size();
			ret = false;

			if ( rand == 0 ) {
				// SET
				if ( size < numRecords ) {
					// Construct new key-value pair
					keySize = Main.keySize >> 3;
					valueSize = Main.fixedSize ? ( Main.chunkSize >> 5 ) : this.random.nextInt( Main.chunkSize >> 3 );
					if ( valueSize < 4 ) valueSize = 4;

					// Store it in the HashMap
					do {
						key = this.generate( keySize );
					} while ( this.map.containsKey( key ) );
					value = this.generate( valueSize );

					// Issue the request
					ret = plio.set( key, value );

					if ( ret ) this.map.put( key, value );

					this.completed[ 0 ]++;
					if ( ret ) this.succeeded[ 0 ]++;
					this.incrementCounter();
					i++;
				}
			} else if ( size > 0 ) {
				// Retrieve one key-value pair in the HashMap
				Object[] entries;
				Map.Entry<String, String> entry;
				index = this.random.nextInt( size );
				entries = this.map.entrySet().toArray();
				entry = ( Map.Entry<String, String> ) entries[ index ];
				key = entry.getKey();
				value = entry.getValue();
				entries = null;

				if ( rand == 1 ) {
					// GET
					String v = plio.get( key );
					ret = ( v != null );

					if ( ret )
						ret = v.equals( value );

					this.completed[ 1 ]++;
					if ( ret ) this.succeeded[ 1 ]++;
					this.incrementCounter();
					i++;
				} else if ( rand == 2 ) {
					// UPDATE
					int length, offset;

					do {
						length = value.length();
						offset = this.random.nextInt( length );
						length = this.random.nextInt( length - offset );
					} while ( length <= 0 );
					String valueUpdate = this.generate( length );
					ret = plio.update( key, valueUpdate, offset );

					if ( ret ) {
						byte[] buffer = value.getBytes();
						byte[] valueUpdateBytes = valueUpdate.getBytes();

						for ( int j = 0; j < length; j++ )
							buffer[ offset + j ] = valueUpdateBytes[ j ];

						value = new String( buffer );
						this.map.put( key, value );
					}

					this.completed[ 2 ]++;
					if ( ret ) this.succeeded[ 2 ]++;
					this.incrementCounter();
					i++;
				} else if ( rand == 3 ) {
					// DELETE
					ret = plio.delete( key );

					if ( ret ) this.map.remove( key );

					// Test whether the key is still available
					if ( plio.get( key ) != null )
						ret = false;

					this.completed[ 3 ]++;
					if ( ret ) this.succeeded[ 3 ]++;
					this.incrementCounter();
					i++;
				}
			}
		}
	}

	public static void main( String[] args ) throws Exception {
		if ( args.length < 7 ) {
			System.err.println( "Required parameters: [Maximum Key Size] [Chunk Size] [Hostname] [Port Number] [Number of records] [Number of threads] [Number of operations] [Fixed size? (true/false)]" );
			System.exit( 1 );
		}

		try {
			Main.keySize = Integer.parseInt( args[ 0 ] );
			Main.chunkSize = Integer.parseInt( args[ 1 ] );
			Main.host = args[ 2 ];
			Main.port = Integer.parseInt( args[ 3 ] );
			Main.numRecords = Integer.parseInt( args[ 4 ] );
			Main.numThreads = Integer.parseInt( args[ 5 ] );
			Main.numOps = Integer.parseInt( args[ 6 ] );
		} catch( NumberFormatException e ) {
			System.err.println( "Parameters: [Maximum Key Size], [Chunk Size], [Port Number], [Number of records], [Number of threads], and [Number of operations] should be integers." );
			System.exit( 1 );
			return;
		}
		fixedSize = args[ 7 ].equals( "true" );

		/* Initialization */
		Main.mainObjs = new Main[ Main.numThreads ];
		Main.threads = new Thread[ Main.numThreads ];
		Main.lock = new Object();
		for ( int i = 0; i < Main.numThreads; i++ ) {
			int startId = Integer.MAX_VALUE / Main.numThreads * i;
			Main.mainObjs[ i ] = new Main( startId );
			Main.threads[ i ] = new Thread( Main.mainObjs[ i ] );
		}

		/* Start execution */
		for ( int i = 0; i < numThreads; i++ ) {
			Main.threads[ i ].start();
		}

		for ( int i = 0; i < numThreads; i++ ) {
			Main.threads[ i ].join();
		}

		/* Report statistics */
		int[] completed = new int[ 4 ];
		int[] succeeded = new int[ 4 ];
		for ( int i = 0; i < numThreads; i++ ) {
			for ( int j = 0; j < 4; j++ ) {
				completed[ j ] += Main.mainObjs[ i ].completed[ j ];
				succeeded[ j ] += Main.mainObjs[ i ].succeeded[ j ];
			}
		}

		System.out.printf(
			"\n" +
			"Number of SET operations    : %d / %d\n" +
			"Number of GET operations    : %d / %d\n" +
			"Number of UPDATE operations : %d / %d\n" +
			"Number of DELETE operations : %d / %d\n",
			succeeded[ 0 ], completed[ 0 ],
			succeeded[ 1 ], completed[ 1 ],
			succeeded[ 2 ], completed[ 2 ],
			succeeded[ 3 ], completed[ 3 ]
		);
	}
}
