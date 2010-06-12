<?php
// $Id$

/**
 * Simple caching of query results for Views displays.
 * Includes listening for changes/posts/deletions of
 * certain node types.
 */
class views_content_cache_plugin_cache extends views_plugin_cache {
  function option_defaults(&$options) {
    $options['min_lifespan'] = 60;
    $options['max_lifespan'] = 21600;
  }

  function options_form(&$form, &$form_state) {
    //node type options
    $types = node_get_types();
	$options = array();
	foreach ( $types as $type)
		$options[$type->type] = $type->name;

	$form['node_types'] = array(
      '#type' => 'select',
      '#title' => t('Flush on node update'),
      '#description' => t('Flush cache when nodes of this type are updated/changed/deleted.'),
      '#multiple' => TRUE,
      '#options' => $options,
      '#default_value' => $this->options['node_types'],
    );

    //Intervall options
    $options = array(60, 300, 1800, 3600, 21600, 518400);
    $options = drupal_map_assoc($options, 'format_interval');
    $options = array(-1 => t('None')) + $options;
    $form['min_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Minimum Lifespan'),
      '#description' => t('Force a minimum cache lifespan.'),
      '#options' => $options,
      '#default_value' => $this->options['min_lifespan'],
    );
    $form['max_lifespan'] = array(
      '#type' => 'select',
      '#title' => t('Maximum Lifespan'),
      '#description' => t('The maximum lifespan, even if nothing else has changed.'),
      '#options' => $options,
      '#default_value' => $this->options['max_lifespan'],
    );

  }

  function summary_title() {
    return "Advanced Cache";
  }

  function cache_expire($type) {

	//check for most recent update
	$lastupdate = 0;
	foreach ( $this->options['node_types'] as $typetype => $typename)
	{
		$update = variable_get ( "views_advanced_cache_typeupdate_".$typetype, 0 );
		$lastupdate = max ($update, $lastupdate);
		drupal_set_message ($lastupdate);

	}

	//calculate min and max cutoff
	$min_cutoff = time() - $this->options['min_lifespan'];
	$max_cutoff = time() - $this->options['max_lifespan'];

	$cutoff = min ( $min_cutoff, $lastupdate );
	$cutoff = max ( $max_cutoff, $cutoff );

	//that's it
	return $cutoff;
  }

}
