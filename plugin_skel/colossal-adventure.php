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

function ca_enqueue_scripts() {
    $asset_base = plugins_url( 'assets/', __FILE__ );

    // Emscripten glue + wasm/data loader
    wp_enqueue_script(
        'colossal-adventure-module',
        plugins_url( 'assets/advent.js', __FILE__ ),
        array(),
        '1.0.0',
        true
    );

    // Our UI + runner
    wp_enqueue_script(
        'colossal-adventure-play',
        plugins_url( 'assets/play.js', __FILE__ ),
        array( 'colossal-adventure-module' ),
        '1.0.0',
        true
    );
    wp_script_add_data( 'colossal-adventure-play', 'type', 'module' );

    // Pass the asset base URL down to JS
    wp_localize_script(
        'colossal-adventure-play',
        'ColossalAdventureConfig',
        array(
            'assetBaseUrl' => trailingslashit( $asset_base ),
        )
    );
}
add_action( 'wp_enqueue_scripts', 'ca_enqueue_scripts' );

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
    ob_start();
    ?>
    <div class="advent-game-container" data-advent-root></div>
    <?php
    return ob_get_clean();
}
add_shortcode( 'advent_game', 'ca_advent_shortcode' );
