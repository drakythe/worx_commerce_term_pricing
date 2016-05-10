<?php

function worx_commerce_term_pricing_settings_form() {
  $temp_thing = array('BULK01');

  global $user;
  $order = commerce_cart_order_load($user->uid);
  foreach ($temp_thing as $sku) {
    $product = commerce_product_load_by_sku($sku);
    dsm($product);
    dsm($order);
  }


  $form['contact_information'] = array(
    '#markup' => t('You can leave us a message using the contact form below.'),
  );

  return $form;
}
