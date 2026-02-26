<?php
/**
 * Plugin Name: Colossal Adventure (WASM)
 * Description: Classic Colossal Cave Adventure via WebAssembly. Use [advent_game].
 * Version:     1.0.0
 * Author:      Dave Rowntree
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

function ca_asset_version( $relative_path ) {
    $full_path = plugin_dir_path( __FILE__ ) . ltrim( $relative_path, '/' );

    if ( file_exists( $full_path ) ) {
        $mtime = filemtime( $full_path );
        if ( false !== $mtime ) {
            return (string) $mtime;
        }
    }

    return '1.0.0';
}

function ca_register_scripts() {
    wp_register_script(
        'colossal-adventure-module',
        plugins_url( 'assets/advent.js', __FILE__ ),
        array(),
        ca_asset_version( 'assets/advent.js' ),
        true
    );

    wp_register_script(
        'colossal-adventure-play',
        plugins_url( 'assets/play.js', __FILE__ ),
        array( 'colossal-adventure-module' ),
        ca_asset_version( 'assets/play.js' ),
        true
    );

    wp_script_add_data( 'colossal-adventure-play', 'type', 'module' );
}
add_action( 'wp_enqueue_scripts', 'ca_register_scripts' );

function ca_enqueue_shortcode_assets() {
    static $did_enqueue = false;

    if ( $did_enqueue ) {
        return;
    }

    $did_enqueue = true;

    $asset_base = trailingslashit( plugins_url( 'assets/', __FILE__ ) );

    wp_enqueue_script( 'colossal-adventure-module' );
    wp_enqueue_script( 'colossal-adventure-play' );

    // Pass the asset base URL down to JS only when a shortcode instance is rendered.
    wp_localize_script(
        'colossal-adventure-play',
        'ColossalAdventureConfig',
        array(
            'assetBaseUrl' => $asset_base,
        )
    );
}

function ca_script_loader_tag( $tag, $handle, $src ) {
    if ( 'colossal-adventure-play' !== $handle ) {
        return $tag;
    }

    if ( false === strpos( $tag, 'type=' ) ) {
        $tag = str_replace( '<script ', '<script type="module" ', $tag );
    }

    return $tag;
}
add_filter( 'script_loader_tag', 'ca_script_loader_tag', 10, 3 );

function ca_advent_shortcode( $atts = array(), $content = '' ) {
    ca_enqueue_shortcode_assets();

    ob_start();
    ?>
    <div class="advent-game-container" data-advent-root></div>
    <?php
    return ob_get_clean();
}
add_shortcode( 'advent_game', 'ca_advent_shortcode' );
