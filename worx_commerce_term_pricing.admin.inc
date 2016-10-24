<?php

/**
 * @TODO: The form should list the fields available on the line item type
 * attached to the node. Once selected and submitted, create option fields.
 * For text fields, a label and a required status
 * For term_lists, an option set to set which options are available and a
 *  required status
 * For image upload field, a max number of images dropdown.
 */

/**
 * @param $form
 *  Array
 * @param $form_state
 *  Array
 * @param $node
 *  Array
 * @return mixed
 *  Array
 */
function worx_commerce_term_pricing_settings_form($form, &$form_state, $node) {
  $term_options = $available_products = array();
  // @TODO: This needs to be a var set somewhere.
  $product_field = 'field_product_display_product';


  $info = field_info_instances('node', $node->type);

  $line_item_type = $info[$product_field]['display']['default']['settings']['line_item_type'];
  $line_item_info = field_info_instances('commerce_line_item', $line_item_type);

  foreach ($node->{$product_field}[LANGUAGE_NONE] as $delta => $pid) {
    $product = commerce_product_load($pid['product_id']);
    $available_products[$product->sku] = $product->title;
  }
  $available_options = db_select('commerce_term_pricing_node_options', 'ctpno')
    ->fields('ctpno')
    ->condition('nid', $node->nid)
    ->execute()
    ->fetchAssoc();
  $available_options = unserialize($available_options['options_data']);
  $field_options = array();
  foreach ($available_options['available_fields'] as $field_id_key => $field_id) {
    if (!isset($available_options[$field_id . '_options'])) {
      continue;
    }
    $field_options[$field_id_key] = $field_id;
  }

  // Render a table of the existing rules
  $rules_table = worx_commerce_term_pricing_get_rules($node->nid);

  $form['rules_table'] = array(
    '#markup' => $rules_table,
  );
  $form['nid'] = array(
    '#type' => 'textfield',
    '#title' => t('Node'),
    '#default_value' => $node->nid,
    '#size' => 25,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#access' => FALSE,
  );
  $form['add_pricing_rule'] = array(
    '#markup' => t("Add New Pricing Rule"),
    '#prefix' => '<strong>',
    '#suffix' => '</strong>',
  );
  $form['product'] = array(
    '#type' => 'select',
    '#title' => t('Product'),
    '#options' => $available_products,
    '#required' => TRUE,
    '#default_value' => '',
    '#prefix' => '<table><tr><td>',
    '#suffix' => '</td>',
   );
  $form['attribute_field'] = array(
    '#type' => 'select',
    '#title' => t('Field'),
    '#options' => $field_options,
    '#required' => TRUE,
    '#empty_option' => t('-None-'),
    '#default_value' => array(),
    '#prefix' => '<td>',
    '#suffix' => '</td>',
  );
  foreach ($field_options as $field_id) {
    $enabled_options = array_filter($available_options[$field_id . '_options']);

    switch ($line_item_info[$field_id]['display']['default']['type']) {
      case 'list_default':
        $list_options = field_info_field($field_id)['settings']['allowed_values'];
        $field_id_options = array_intersect_key($list_options, $enabled_options);
        break;
      case 'taxonomy_term_reference_link':
        $term_options = array();
        $term_vocab = taxonomy_vocabulary_machine_name_load(field_info_field($field_id)['settings']['allowed_values'][0]['vocabulary']);
        $vocab_tree = taxonomy_get_tree($term_vocab->vid);
        foreach ($vocab_tree as $term) {
          $term_options[$term->tid] = $term->name;
        }
        $field_id_options = array_intersect_key($term_options, $enabled_options);
        break;
    }
    $form[$field_id . '_attribute'] = array(
      '#type' => 'select',
      '#title' => t('Attribute'),
      '#options' => $field_id_options,
      '#empty_option' => t('-None-'),
      '#default_value' => array(),
      '#prefix' => '<td>',
      '#suffix' => '</td>',
      '#states' => array(
        'visible' => array(
          ':input[name="attribute_field"]' => array('value' => $field_id),
        ),
        'required' => array(
          ':input[name="attribute_field"]' => array('value' => $field_id),
        ),
      ),
    );
  }
  $form['change'] = array(
    '#type' => 'textfield',
    '#title' => t('Price Change'),
    '#default_value' => '',
    '#size' => 25,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#prefix' => '<td>',
    '#suffix' => '</td>',
  );
  $form['change_type'] = array(
    '#type' => 'select',
    '#title' => t('Price Change Type'),
    '#options' => array(
      'flat' => 'Flat Increase/decrease',
      'percent' => 'Percent Change',
    ),
    '#default_value' => array('flat'),
    '#prefix' => '<td>',
    '#suffix' => '</td></tr></table>',
  );

  $form['#validate'][] = 'worx_commerce_term_pricing_rule_validate';

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  return $form;
}

/**
 * @param $form
 *  Array
 * @param $form_state
 *  Array
 */
function worx_commerce_term_pricing_rule_validate($form, &$form_state) {
  $values = $form_state['values'];

  if (!is_numeric($values['change'])) {
    form_set_error('change', 'Please set a valid amount for price change');
  }

  $valid = FALSE;

  $attribute_set = $values['attribute_field'];

  if (!empty($values[$attribute_set . '_attribute'])) {
    $valid = TRUE;
  }

  if (!$valid) {
    form_set_error($attribute_set . '_attribute', 'Please select an attribute to apply this pricing rule to.');
  }
}

/**
 * @param $form
 *  Array
 * @param $form_state
 *  Array
 */
function worx_commerce_term_pricing_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $record = array();
  $update = FALSE;
  $attribute_field = $values['attribute_field'];


  $record['product_sku'] = $values['product'];
  $record['price_change'] = $values['change'];
  $record['attributes'][$attribute_field] = $values[$attribute_field . '_attribute'];
  $record['change_type'] = $values['change_type'];
  $record['nid'] = $values['nid'];

  $query = db_select('commerce_term_pricing_rules', 'ctpr')
    ->fields('ctpr')
    ->condition('product_sku', $values['product'])
    ->execute();

  while ($result = $query->fetchAssoc()) {
    $attribute = unserialize($result['attributes']);
    if ($attribute == $record['attributes']) {
      $update = TRUE;
      $record['tprid'] = $result['tprid'];
      break;
    }
  }

  if ($update) {
    $write = drupal_write_record('commerce_term_pricing_rules', $record, 'tprid');
  }
  else {
    $write = drupal_write_record('commerce_term_pricing_rules', $record);
  }

  if ($write === FALSE) {
    drupal_set_message('Rule failed to write to Database, inform site Administrator', 'error');
    watchdog('Term Pricing', '<pre>' . print_r($record, TRUE) . '</pre>', array(), WATCHDOG_ERROR);
  }
}

/**
 * @param $nid
 *  Int
 * @return string
 */
function worx_commerce_term_pricing_get_rules($nid) {
  $header = array(
    array('data' => 'Product', 'field' => 'product_sku', 'sort' => 'ASC'),
    array('data' => 'Attribute', 'field' => 'attributes', 'sort' => 'ASC'),
    array('data' => 'Change', 'field' => 'price_change', 'sort' => 'ASC'),
    array('data' => 'Change Type', 'field' => 'change_type'),
    array('data' => 'Delete', 'field' => 'tprid'),
  );

  $select = db_select('commerce_term_pricing_rules', 'ctpr')
    ->extend('PagerDefault')
    ->extend('TableSort');

  $select->fields('ctpr', array('tprid', 'product_sku', 'attributes', 'price_change', 'change_type'))
    ->condition('nid', $nid)
    ->limit(25)
    ->orderByHeader($header);

  $results = $select->execute();

  $rows = array();
  foreach ($results as $row) {
    if (!empty($row->product_sku)) {
      $product = commerce_product_load_by_sku($row->product_sku);
      $title = $product->title;
      $title = $title . '(' . $row->product_sku . ')';
    }
    else {
      $title = 'N/A';
    }

    $node = node_load($nid);
    $attribute_name = '';
    $attribute_array = unserialize($row->attributes);

    $info = field_info_instances('node', $node->type);
    //@TODO Set as var in future.
    $product_field = 'field_product_display_product';
    $line_item_type = $info[$product_field]['display']['default']['settings']['line_item_type'];
    $line_item_info = field_info_instances('commerce_line_item', $line_item_type);
    reset($attribute_array);
    $field_id = key($attribute_array);
    switch ($line_item_info[$field_id]['display']['default']['type']) {
      case 'list_default':
        $list_options = field_info_field($field_id)['settings']['allowed_values'];
        $attribute_name = $list_options[$attribute_array[$field_id]];
        break;
      case 'taxonomy_term_reference_link':
        $term_options = array();
        $term_vocab = taxonomy_vocabulary_machine_name_load(field_info_field($field_id)['settings']['allowed_values'][0]['vocabulary']);
        $vocab_tree = taxonomy_get_tree($term_vocab->vid);
        foreach ($vocab_tree as $term) {
          $term_options[$term->tid] = $term->name;
        }
        $attribute_name = $term_vocab->name . ': ' .$term_options[$attribute_array[$field_id]];
        break;
    }

    $rows[] = array(
      $title,
      $attribute_name,
      $row->price_change,
      $row->change_type, 
      l('Delete', 'admin/commerce/term_pricing/rule/' . $row->tprid . '/delete', array('query' => array('destination' => '/node/' .$nid . '/term_pricing/rule')))
    );
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');

  return $output;
}

/**
 * @return array
 */
function worx_commerce_term_pricing_options_form() {
  $form = $node_options = $saved_options = array();
  // @TODO: Make 'product_display' a variable that can be configured.
  $nodes = node_load_multiple(array(), array('type' => 'product_display'));
  foreach ($nodes as $node) {
    $node_options[$node->nid] = $node->title;
  }
  $saved_nodes = db_select('commerce_term_pricing_node_options', 'ctpno')
    ->fields('ctpno')
    ->execute();

  while ($option_set = $saved_nodes->fetchAssoc()) {
    if (isset($option_set['options_data'])) {
      $saved_options[$option_set['nid']] = $option_set['options_data'];
    }
  }

  $unsaved_options = array_diff_key($node_options, $saved_options);

  $table = worx_commerce_term_pricing_options_create_table($unsaved_options, $saved_options);

  $form['markup'] = array(
    '#markup' => $table,
  );

  return $form;
}

/**
 * @param $unsaved_options
 * @param $saved_options
 * @return string
 */
function worx_commerce_term_pricing_options_create_table($unsaved_options, $saved_options) {
  $rows = array();
  $header = array(
    array('data' => 'Product', 'field' => 'product', 'sort' => 'ASC'),
    array('data' => 'Edit', 'field' => 'edit'),
    array('data' => 'Delete', 'field' => 'tprid'),
  );

  foreach ($saved_options as $nid => $options) {
    $node = node_load($nid);
    // Make sure the NID returned a valid node.
    if (!$node) {
      continue;
    }
    $title = $node->title;
    $rows[] = array(
      $title,
      l("Edit Option Set Rule", 'admin/commerce/term_pricing/options/' . $nid),
      l("Delete Option Set Rule", 'admin/commerce/term_pricing/options/' . $nid . '/delete', array('query' => array('destination' => '/admin/commerce/term_pricing/options'))),
    );
  }
  foreach ($unsaved_options as $nid => $title) {
    $rows[] = array(
      $title,
      l("Add Option Set Rule", 'admin/commerce/term_pricing/options/' . $nid),
      "No option set created",
    );
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');

  return $output;
}

/**
 * Creates the option set form for determining available attributes on a node.
 *
 * @param $form
 *  Array
 * @param $form_id
 *  Array
 * @param $node
 *  Object
 * @return array
 */
function worx_commerce_term_pricing_node_option_form($form, &$form_id, $node) {
  $form = $possible_fields = $list_fields = $term_fields = $text_fields = $image_fields = array();

  // @TODO: This needs to be a var set somewhere.
  $product_field = 'field_product_display_product';

  $info = field_info_instances('node', $node->type);
  $line_item_type = $info[$product_field]['display']['default']['settings']['line_item_type'];
  $line_item_info = field_info_instances('commerce_line_item', $line_item_type);

  foreach ($line_item_info as $field => $properties) {
    // Setup filter arrays.
    $no_options = array(
      'commerce_display_path',
    );
    $optionable_types = array(
      'list_default',
      'taxonomy_term_reference_link',
      'text_default',
      'image',
    );

    // This will handle default fields which are the same type as fields we
    // want to create options for.
    if (in_array($field, $no_options)) {
      continue;
    }

    // Now we check to make sure we can handle the field type.
    if (in_array($properties['display']['default']['type'], $optionable_types)
    && $properties['commerce_cart_settings']['field_access']) {
      //dsm(field_info_field($field));
      $possible_fields[$field] = $properties['label'];

      switch ($properties['display']['default']['type']) {
        case 'list_default':
          $list_fields[$field] = $properties['label'];
          break;
        case 'taxonomy_term_reference_link':
          $term_fields[$field] = $properties['label'];
          break;
        case 'text_default':
          $text_fields[$field] = $properties['label'];
          break;
        case 'image':
          $image_fields[$field] = $properties['label'];
          break;
      }
    }
  }
  $defaults = db_select('commerce_term_pricing_node_options', 'ctpno')
    ->fields('ctpno')
    ->condition('nid', $node->nid)
    ->execute()
    ->fetchAssoc();
  $defaults = unserialize($defaults['options_data']);
  isset($defaults['available_fields']) ? $set_lists = $defaults['available_fields'] : $set_lists = array();

  // Now that we have all the pieces we actually create the form!
  // Need the nid for saving into the DB.
  $form['nid'] = array(
    '#type' => 'textfield',
    '#title' => t('Node'),
    '#default_value' => $node->nid,
    '#size' => 25,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#access' => FALSE,
  );
  // Allow the user to select which attributes are available whne adding to cart
  $form['available_fields'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Available Attribute Fields'),
    '#description' => t('Select a field if it should be available as an attribute when adding a product to the cart.'),
    '#options' => $possible_fields,
    '#empty_option' => t('-None-'),
    '#default_value' => $set_lists,
  );

  // Each field type has specific fields, so we loop through the active ones
  // and create them.
  foreach ($list_fields as $list_field => $list_label) {
    if (array_key_exists($list_field, $set_lists)) {
      $form[$list_field] = array(
        '#type' => 'fieldset',
        '#title' => t($list_label . ' Setting'),
        '#collapsible' => TRUE, // Added
        '#collapsed' => TRUE,  // Added
      );
      $form[$list_field][$list_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($list_label . ' Field Required?.'),
        '#default_value' => isset($defaults[$list_field . '_required']) ? $defaults[$list_field . '_required'] : FALSE,
      );
      $form[$list_field][$list_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($list_label . ' field Label'),
        '#default_value' => isset($defaults[$list_field . '_label']) ? $defaults[$list_field . '_label'] : $list_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
      //Get list options and allow users to select availability
      $list_options = field_info_field($list_field)['settings']['allowed_values'];
      $form[$list_field][$list_field . '_options'] = array(
        '#type' => 'checkboxes',
        '#title' => t($list_label . ' Options'),
        '#options' => $list_options,
        '#default_value' => isset($defaults[$list_field . '_options']) ? $defaults[$list_field . '_options'] : array(),
        '#description' => t('Select which options are available for this attribute on this product.'),
      );
    }
  }

  foreach ($term_fields as $term_field => $term_label) {
    if (array_key_exists($term_field, $set_lists)) {
      $form[$term_field] = array(
        '#type' => 'fieldset',
        '#title' => t($term_label . ' Setting'),
        '#collapsible' => TRUE, // Added
        '#collapsed' => TRUE,  // Added
      );
      $form[$term_field][$term_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($term_label . ' Field Required?.'),
        '#default_value' => isset($defaults[$term_field . '_required']) ? $defaults[$term_field . '_required'] : FALSE,
      );
      $form[$term_field][$term_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($term_label . ' field Label'),
        '#default_value' => isset($defaults[$term_field . '_label']) ? $defaults[$term_field . '_label'] : $term_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
      $term_options = array();
      $term_vocab = taxonomy_vocabulary_machine_name_load(field_info_field($term_field)['settings']['allowed_values'][0]['vocabulary']);
      $vocab_tree = taxonomy_get_tree($term_vocab->vid);
      foreach ($vocab_tree as $term) {
        $term_options[$term->tid] = $term->name;
      }
      $form[$term_field][$term_field . '_options'] = array(
        '#type' => 'checkboxes',
        '#title' => t($term_label . ' Options'),
        '#options' => $term_options,
        '#default_value' => isset($defaults[$term_field . '_options']) ? $defaults[$term_field . '_options'] : array(),
        '#description' => t('Select which options are available for this attribute on this product.'),
      );
    }
  }
  
  foreach ($text_fields as $text_field => $text_label) {
    if (array_key_exists($text_field, $set_lists)) {
      $form[$text_field] = array(
        '#type' => 'fieldset',
        '#title' => t($text_label . ' Setting'),
        '#collapsible' => TRUE, // Added
        '#collapsed' => TRUE,  // Added
      );
      $form[$text_field][$text_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($text_label . ' Field Required?.'),
        '#default_value' => isset($defaults[$text_field . '_required']) ? $defaults[$text_field . '_required'] : FALSE,
      );
      $form[$text_field][$text_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($text_label . ' field Label'),
        '#default_value' => isset($defaults[$text_field . '_label']) ? $defaults[$text_field . '_label'] : $text_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
    }
  }
  
  foreach ($image_fields as $image_field => $image_label) {
    if (array_key_exists($image_field, $set_lists)) {
      $form[$image_field] = array(
        '#type' => 'fieldset',
        '#title' => t($image_label . ' Setting'),
        '#collapsible' => TRUE, // Added
        '#collapsed' => TRUE,  // Added
      );
      $form[$image_field][$image_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($image_label . ' Field Required?.'),
        '#default_value' => isset($defaults[$image_field . '_required']) ? $defaults[$image_field . '_required'] : FALSE,
      );
      $form[$image_field][$image_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($image_label . ' field Label'),
        '#default_value' => isset($defaults[$image_field . '_label']) ? $defaults[$image_field . '_label'] : $image_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
      $form[$image_field][$image_field . '_max'] = array(
        '#type' => 'select',
        '#title' => t('Maximum Number of Images'),
        '#options' => range(0, 1),
        '#empty_option' => t('-none-'),
        '#required' => FALSE,
        '#default_value' => isset($defaults[$image_field . '_max']) ? $defaults[$image_field . '_max'] : array(),
      );
    }
  }

  $form['#validate'][] = 'worx_commerce_term_pricing_options_validate';
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  return $form;
}

function worx_commerce_term_pricing_options_validate($form, $form_state) {
}

function worx_commerce_term_pricing_node_option_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $nid = $values['nid'];
  $lists = array_filter($values['available_fields']);
  unset($values['submit'], $values['form_build_id'], $values['form_token'],
    $values['form_id'], $values['op'], $values['nid'],
    $values['available_fields']);

  $exists = db_select('commerce_term_pricing_node_options', 'ctpno')
    ->fields('ctpno')
    ->condition('nid', $nid)
    ->execute()
    ->fetchAssoc();

  $record = array(
    'nid' => $nid,
    'options_data' => array(
      'available_fields' => $lists,
    ),
  );
  
  foreach ($values as $field_name => $field_option_values) {
    $record['options_data'][$field_name] = $field_option_values;
  }

  if (!empty($exists)) {
    drupal_write_record('commerce_term_pricing_node_options', $record, 'nid');
  }
  else {
    drupal_write_record('commerce_term_pricing_node_options', $record);
  }
}
