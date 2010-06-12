<?php
// $Id$

/**
 * Simple caching of query results for Views displays.
 * Includes listening for changes/posts/deletions of
 * certain node types.
 */
class views_content_cache_plugin_cache extends views_plugin_cache {
  function option_defaults(&$options) {
    $options['results_min_lifespan'] = 0;
    $options['results_max_lifespan'] = 0;
    $options['output_min_lifespan'] = 21600;
    $options['output_max_lifespan'] = 21600;
    $options['node_types'] = array();
  }

  function options_form(&$form, &$form_state) {
    $types = node_get_types('names');

	  $form['node_types'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Content types to monitor'),
      '#description' => t('The selected content types will be monitored for changes and the cache flushed accordingly.'),
      '#options' => $types,
      '#default_value' => $this->options['node_types'],
    );

    $options = array(60, 300, 1800, 3600, 21600, 518400);
    $options = drupal_map_assoc($options, 'format_interval');
    $options = array(-1 => t('None')) + $options;

    $form['results_min_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Query results - Minimum lifetime'),
      '#description' => t('Ensure results are cached for at least this amount of time.'),
      '#options' => $options,
      '#default_value' => $this->options['results_min_lifespan'],
    );
    $form['results_max_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Query results - Maximum lifetime'),
      '#description' => t('Ensure results are cached for at most this amount of time.'),
      '#options' => $options,
      '#default_value' => $this->options['results_max_lifespan'],
    );

    $form['output_min_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Rendered output - Minimum lifetime'),
      '#description' => t('Ensure rendered HTML output are cached for at least this amount of time.'),
      '#options' => $options,
      '#default_value' => $this->options['output_min_lifespan'],
    );
    $form['output_max_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Rendered output - Maximum lifetime'),
      '#description' => t('Ensure rendered HTML output are cached for at most this amount of time.'),
      '#options' => $options,
      '#default_value' => $this->options['output_max_lifespan'],
    );

  }

  function summary_title() {
    return "Content cache";
  }

  /**
   * Return the expiry time for this cache plugin.
   *
   * This should be the last time that the content has changed, altered to allow
   * for the the min/max lifetimes.
   */
  function cache_expire($type) {

    $cutoff = 0;
    foreach (array_filter($this->options['node_types']) as $content_type => $v) {
      $cutoff = max($cutoff, views_content_cache_last_update_get($content_type));
    }

    // If there's a minimum lifetime, enforce it:
    if ($min_lifespan = $this->options[$type . '_min_lifespan']) {
      $min_lifespan = time() - $min_lifespan;
      $cutoff = min($min_lifespan, $cutoff);
    }

    // Only enforce a maximum lifetime if it's been specifically selected:
    if ($max_lifespan = $this->options[$type . '_max_lifespan']) {
      if ($max_lifespan != -1) {
        $max_lifespan = time() - $max_lifespan;
        $cutoff = max($max_lifespan, $cutoff);
      }
    }

    return $cutoff;
  }

}
