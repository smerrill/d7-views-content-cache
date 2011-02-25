<?php

/**
 * Simple caching of query results for Views displays.
 * Includes listening for changes/posts/deletions of
 * certain node types.
 */
class views_content_cache_plugin_cache extends views_plugin_cache {
  function option_definition() {
    $options = parent::option_definition();

    $options['results_min_lifespan'] = array('default' => 0);
    $options['results_max_lifespan'] = array('default' => 21600);
    $options['output_min_lifespan'] = array('default' => 0);
    $options['output_max_lifespan'] = array('default' => 21600);
    $options['keys'] = array('default' => array());

    return $options;
  }

  // We seem to need this for views 2
  function option_defaults(&$options) {
    $options['results_max_lifespan'] = 21600;
    $options['output_max_lifespan'] = 21600;
  }

  function options_form(&$form, &$form_state) {
    $form['keys'] = array(
      '#title' => t('Cache segments'),
      '#type' => 'fieldset',
      '#description' => t("Choose which events will update this View's cache."),
    );
    foreach (views_content_cache_get_plugin() as $key_id => $plugin) {
      $option_value = isset($this->options['keys'][$key_id]) ? $this->options['keys'][$key_id] : array();
      $form['keys'][$key_id] = $plugin->options_form($option_value, $this);
    }

    $options = array(60, 300, 1800, 3600, 21600, 518400);
    $options = drupal_map_assoc($options, 'format_interval');
    $options = array(-1 => t('None')) + $options;

    $form['results_min_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Query results - Minimum lifetime'),
      '#description' => t('Query results will be cached for <strong>at least</strong> this amount of time, even if a selected event has occured more recently.'),
      '#options' => $options,
      '#default_value' => $this->options['results_min_lifespan'],
    );
    $form['results_max_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Query results - Maximum lifetime'),
      '#description' => t('Query results will be cached for <strong>at most</strong> this amount of time.'),
      '#options' => $options,
      '#default_value' => $this->options['results_max_lifespan'],
    );

    $form['output_min_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Rendered output - Minimum lifetime'),
      '#description' => t('Rendered HTML output will be cached for <strong>at least</strong> this amount of time, even if a selected event has occured more recently.'),
      '#options' => $options,
      '#default_value' => $this->options['output_min_lifespan'],
    );
    $form['output_max_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Rendered output - Maximum lifetime'),
      '#description' => t('Rendered HTML output will be cached for <strong>at most</strong> this amount of time.'),
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

    // Retrieve the latest update time matching the settings on this View.
    $cid = array();
    if (!empty($this->options['keys'])) {
      foreach ($this->options['keys'] as $key_id => $key_values) {
        if ($plugin = views_content_cache_get_plugin($key_id)) {
          $cid[$key_id] = $plugin->view_key($key_values, $this);
        }
      }
    }
    if (!empty($cid) && $timestamp = views_content_cache_update_get($cid)) {
      $cutoff = $timestamp;
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


  function get_results_key() {
    global $user;

    if (!isset($this->_results_key)) {

      // This is a fix for the build info not including the actual query run:
      // See:
      $execute_info = $this->view->build_info;

      $execute_info['query'] = db_rewrite_sql($execute_info['query'], $this->view->base_table, $this->view->base_field, array('view' => &$this->view));
      $execute_info['count_query'] = db_rewrite_sql($execute_info['count_query'], $this->view->base_table, $this->view->base_field, array('view' => &$this->view));
      $replacements = module_invoke_all('views_query_substitutions', $this->view);
      $execute_info['query'] = str_replace(array_keys($replacements), $replacements, $execute_info['query']);
      $execute_info['count_query'] = 'SELECT COUNT(*) FROM (' . str_replace(array_keys($replacements), $replacements, $execute_info['count_query']) . ') count_alias';
      // End generating the execution info.

      $key_data = array(
        'execute_info' => $execute_info,
        'roles' => array_keys($user->roles),
        'super-user' => $user->uid == 1, // special caching for super user.
        'language' => $GLOBALS['language'],
      );
      foreach (array('exposed_info', 'page', 'sort', 'order') as $key) {
        if (isset($_GET[$key])) {
          $key_data[$key] = $_GET[$key];
        }
      }

      $this->_results_key = $this->view->name . ':' . $this->display->id . ':results:' . md5(serialize($key_data));
    }

    return $this->_results_key;
  }
}
