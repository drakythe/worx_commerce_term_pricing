<?php

function worx_commerce_term_pricing_settings_form() {
  $temp_thing = array('BULK01');

  $flavor = taxonomy_vocabulary_machine_name_load('product_flavor');
  $fl_terms = entity_load('taxonomy_term', FALSE, array('vid' => $flavor->vid));
  $strength = taxonomy_vocabulary_machine_name_load('product_strength');
  $st_terms = entity_load('taxonomy_term', FALSE, array('vid' => $strength->vid));
  $pgvg = taxonomy_vocabulary_machine_name_load('product_pg_vg');
  $pgvg_terms = entity_load('taxonomy_term', FALSE, array('vid' => $pgvg->vid));
  dsm($fl_terms);
  dsm($st_terms);
  dsm($pgvg_terms);

  global $user;
  $order = commerce_cart_order_load($user->uid);
  foreach ($temp_thing as $sku) {
    $product = commerce_product_load_by_sku($sku);
    dsm($product);
  }
  foreach ($order->commerce_line_items[LANGUAGE_NONE] as $index => $array) {
    $line_item = commerce_line_item_load($array['line_item_id']);
    dsm($line_item);
  }
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'commerce_product')
    ->entityCondition('bundle', 'product')
    ->propertyCondition('status', 1)
    ->fieldOrderBy('commerce_price', 'amount', 'ASC')
  ;
  $result = $query->execute();
  dsm($result);


  $form['contact_information'] = array(
    '#markup' => t('You can leave us a message using the contact form below.'),
  );

  return $form;
}
